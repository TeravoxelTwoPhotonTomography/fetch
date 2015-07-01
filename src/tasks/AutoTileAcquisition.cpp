/** \file
  Task: Full-automatic 3D tiling acquisition.

  \author: Nathan Clack <clackn@janelia.hhmi.org>

  \copyright
  Copyright 2010 Howard Hughes Medical Institute.
  All rights reserved.
  Use is subject to Janelia Farm Research Campus Software Copyright 1.1
  license terms (http://license.janelia.org/license/jfrc_copyright_1_1.html).
 */


#include "common.h"
#include "AutoTileAcquisition.h"
#include "TiledAcquisition.h"
#include "StackAcquisition.h"
#include "Video.h"
#include "frame.h"
#include "devices\digitizer.h"
#include "devices\Microscope.h"
#include "devices\tiling.h"
#include "AdaptiveTiledAcquisition.h"

#define CHKJMP(expr) if(!(expr)) {warning("%s(%d)"ENDL"\tExpression indicated failure:"ENDL"\t%s"ENDL,__FILE__,__LINE__,#expr); goto Error;}
#define WARN(msg)    warning("%s(%d)"ENDL"\t%s"ENDL,__FILE__,__LINE__,msg)
#define DBG(...)     do{debug("%s(%d)"ENDL "\t",__FILE__,__LINE__); debug(__VA_ARGS__);}while(0)

namespace fetch
{

  namespace task
  {

    //
    // AutoTileAcquisition -  microscope task
    //

    namespace microscope {

      /** \class AutoTileAcquisition AutoTileAcquisition.h

      Microscope task fro automatic 3d tiling of a volume.

      The task operates in three phases:

      While zpos in bounds:
      -#  Explore the current slice to determine which tiles to acquire.
         - foreach tile in zone:
           - move to tile
           - acquire single image at probe depth (could use stage system to offset...just need to ensure correct tile is marked)
           - classify image
           - mark tile
         - postprocess marked tiles
           - close
           - dilate1
      -#  Run the TiledAcquisition task to collect those tiles.
         - since TiledAcquisition is a microscope task, it's run function will be directly
           invoked rather than running it asynchronously.
      -#  Run the Cut (vibratome) task to cut the imaged slice off.

      Questions:
      -#  How to define exploration zone? Z limits.
          How to iterate over tiles for exploration.
      -#  Feedback on marking.
          - can see tiles getting marked in view as it happens...view linked to current stage zpos
      -#  User interuption.
      -#  Speed?  How fast can I explore a given area?
          - initially assume I don't need to be efficient (take simple approach)

      */

      //Upcasting
      unsigned int AutoTileAcquisition::config(IDevice *d) {return config(dynamic_cast<device::Microscope*>(d));}
      unsigned int AutoTileAcquisition::run   (IDevice *d) {return run   (dynamic_cast<device::Microscope*>(d));}

      unsigned int AutoTileAcquisition::config(device::Microscope *d)
      {
        return 1; //success
Error:
        return 0;
      }

      static int _handle_wait_for_result(DWORD result, const char *msg)
      {
          return_val_if( result == WAIT_OBJECT_0  , 0 );
          return_val_if( result == WAIT_OBJECT_0+1, 1 );
          Guarded_Assert_WinErr( result != WAIT_FAILED );
          switch(result)
          { case WAIT_ABANDONED_0:   warning("TiledAcquisition: Wait 0 abandoned\r\n\t%s\r\n", msg); break;
            case WAIT_ABANDONED_0+1: warning("TiledAcquisition: Wait 1 abandoned\r\n\t%s\r\n", msg); break;
            case WAIT_TIMEOUT:       warning("TiledAcquisition: Wait timeout\r\n\t%s\r\n", msg);     break;
            default:
            ;
          }
          return -1;
      }

      ///// CLASSIFY //////////////////////////////////////////////////
      template<class T>
      static int _classify(mylib::Array *src, int ichan, double intensity_thresh, double area_thresh)
      { T* data;
        mylib::Array tmp=*src,*image=&tmp;
        if(ichan>=0)
        { image=mylib::Get_Array_Plane(&tmp,ichan);
        }
        data = (T*)image->data;
        size_t i,count=0;
        if(!image->size) return 0;
        for(i=0;i<image->size;++i)
          count+=(data[i]>intensity_thresh);
#if 0
        mylib::Write_Image("classify.tif",image,mylib::DONT_PRESS);
#endif
        DBG("Fraction above thresh: %f\n\t\tintensity thresh: %f\n\t\tarea_thresh: %f\n",
          count/((double)image->size),
          intensity_thresh,
          area_thresh);        
        return (count/((double)image->size))>area_thresh;
      }
      #define CLASSIFY(type_id,type) case type_id: return _classify<type>(image,ichan,intensity_thresh,area_thresh); break
      /**
      \returns 0 if background, 1 if foreground

      Image could be multiple channels.  Channels are assumed to plane-wise.
      */
      static int classify(mylib::Array *image, int ichan, double intensity_thresh, double area_thresh)
      {
        if(image->ndims<3)  // check that there are enough dimensions to select a channel
        { ichan=-1;
        } else if(ichan>=image->dims[image->ndims-1]) // is ichan sane?  If not, use chan 0.
        { ichan=0;
        }

        switch(image->type)
        {
          CLASSIFY( mylib::UINT8_TYPE   ,uint8_t );
          CLASSIFY( mylib::UINT16_TYPE  ,uint16_t);
          CLASSIFY( mylib::UINT32_TYPE  ,uint32_t);
          CLASSIFY( mylib::UINT64_TYPE  ,uint64_t);
          CLASSIFY( mylib::INT8_TYPE    , int8_t );
          CLASSIFY( mylib::INT16_TYPE   , int16_t);
          CLASSIFY( mylib::INT32_TYPE   , int32_t);
          CLASSIFY( mylib::INT64_TYPE   , int64_t);
          CLASSIFY( mylib::FLOAT32_TYPE ,float   );
          CLASSIFY( mylib::FLOAT64_TYPE ,double  );
          default:
            return 0;
        }
      }
      #undef CLASSIFY

      ///// EXPLORE  //////////////////////////////////////////////////

      /** Tests to make sure the cut/image cycle stays in z bounds.

      Only need to test max since stage only moves up as cuts progress.

      */
      static int PlaneInBounds(device::Microscope *dc,float maxz)
      { float x,y,z;
        dc->stage()->getPos(&x,&y,&z);
        return z<maxz;
      }

      /**
      Explores the current plane searching for tiles to image.  A heuristic classifier
      is used to target a tile for imaging based on a single snapshot acquired at a
      given depth (\c dz_um).

      Preconditions:
      - tiles to explore have been labelled as such

      Parameters to get from configuration:
      - dz_um:               zpiezo offset
      - maxz                 stage units(mm)
      - timeout_ms
      - ichan:               channel to use for classification, -1 uses all channels
      - intensity_threshold: use the expected pixel units
      - area_threshold:      0 to 1. The fraction of pixels that must be brighter than intensity threshold.

      \returns 0 if no tiles were targeted for imaging, otherwise 1.
      */
      static int explore(device::Microscope *dc)
      { Vector3f tilepos;
        unsigned any_explorable=0,
                 any_active=0;
        cfg::tasks::AutoTile cfg=dc->get_config().autotile();
        size_t iplane=dc->stage()->getPosInLattice().z();

        device::StageTiling* tiling = dc->stage()->tiling();
        tiling->markAddressable(iplane); // make sure the current plane is marked addressable
        tiling->setCursorToPlane(iplane);

        device::TileSearchContext *ctx=0;
        while(  !dc->_agent->is_stopping()
              && tiling->nextSearchPosition(iplane,cfg.search_radius()/*radius - tiles*/,tilepos,&ctx))
              //&& tiling->nextInPlaneExplorablePosition(tilepos))
        { mylib::Array *im;
          any_explorable=1;
          tilepos[2]=dc->stage()->getTarget().z()*1000.0; // convert mm to um
          DBG("Exploring tile: %6.1f %6.1f %6.1f",tilepos.x(),tilepos.y(),tilepos.z());
          CHKJMP(dc->stage()->setPos(tilepos*0.001)); // convert um to mm
          CHKJMP(im=dc->snapshot(cfg.z_um(),cfg.timeout_ms()));
          tiling->markExplored();

          tiling->markDetected(classify(im,cfg.ichan(),cfg.intensity_threshold(),cfg.area_threshold()));
          mylib::Free_Array(im);
        }
        if(!tiling->updateActive(iplane))
        { WARN("No tiles found to image.\n");
          goto Error;
        }
        if(any_explorable)
          tiling->dilateActive(iplane);
        tiling->fillHolesInActive(iplane);
        if(ctx) delete ctx;
        return 1;
      Error:
        if(ctx) delete ctx;
        return 0;
      }

      unsigned int AutoTileAcquisition::run(device::Microscope *dc)
      { unsigned eflag=0; //success
        cfg::tasks::AutoTile cfg=dc->get_config().autotile();
        TiledAcquisition         nonadaptive_tiling;
        AdaptiveTiledAcquisition adaptive_tiling;
        MicroscopeTask *tile=0;
        Cut cut;

        tile=cfg.use_adaptive_tiling()?((MicroscopeTask*)&adaptive_tiling):((MicroscopeTask*)&nonadaptive_tiling);

        while(!dc->_agent->is_stopping() && PlaneInBounds(dc,cfg.maxz_mm()))
        { 
          if(cfg.use_explore())
            CHKJMP(explore(dc));       // will return an error if no explorable tiles found on the plane
          CHKJMP(   tile->config(dc));
          CHKJMP(0==tile->run(dc));

          /* Assert the trip detector hasn't gone off.  
           * Trip detector will signal acq task and microscope tasks to stop, but 
           * we double check here as extra insurance against any extra cuts.
           */
          CHKJMP(dc->trip_detect.ok());

          CHKJMP(   cut.config(dc));
          CHKJMP(0==cut.run(dc));
        }

Finalize:
        return eflag;
Error:
        eflag=1;
        goto Finalize;
      }

    } // namespace microscope

  }   // namespace task
}     // namespace fetch
