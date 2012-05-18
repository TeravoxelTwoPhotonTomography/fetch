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

#define CHKJMP(expr) if(!(expr)) {warning("%s(%d)"ENDL"\tExpression indicated failure:"ENDL"\t%s"ENDL,__FILE__,__LINE__,#expr); goto Error;}

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
        { mylib::Get_Array_Plane(image,ichan);
        } 
        data = (T*)image->data;
        size_t i,count=0;
        if(!image->size) return 0;
        for(i=0;i<image->size;++i)
          count+=(a[i]>intensity_thresh);
        return (count/((double)a->size))>area_thresh;            
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
          CLASSIFY( UINT8_TYPE   ,uint8_t );
          CLASSIFY( UINT16_TYPE  ,uint16_t);
          CLASSIFY( UINT32_TYPE  ,uint32_t);
          CLASSIFY( UINT64_TYPE  ,uint64_t);
          CLASSIFY( INT8_TYPE    ,uint8_t );
          CLASSIFY( INT16_TYPE   ,uint16_t);
          CLASSIFY( INT32_TYPE   ,uint32_t);
          CLASSIFY( INT64_TYPE   ,uint64_t);
          CLASSIFY( FLOAT32_TYPE ,float   );
          CLASSIFY( FLOAT64_TYPE ,double  );
          default;
            return 0;
        }
      }
      #undef CLASSIFY

      ///// EXPLORE  //////////////////////////////////////////////////
      /**
      Preconditions:
      - tiles to explore have been labelled as such
      
      Parameters to get:
      - dz_um:               zpiezo offset
      - ichan:               channel to use for classification, -1 uses all channels
      - intensity_threshold: use the expected pixel units
      - area_threshold:      0 to 1. The fraction of pixels that must be brighter than intensity threshold.
      
      Functions to implement:
      - dc->setZPiezo(dz_um)
      - dc->snapshot() -> mylib::Array() -- or equiv      
      */
      static int explore(device::Microscope *dc)
      { Vector3f tilepos;        
        Guarded_Assert(dc->__scan_agent.is_runnable());

        /// \todo implement Microscope::setZPiezo(dz_um)
        dc->setZPiezo(dz_um);

        device::StageTiling* tiling = dc->stage()->tiling();
        tiling->resetCursor();
        while(  !dc->_agent->is_stopping() 
              && tiling->nextInPlaneExplorablePosition(&tilepos))
        { if(classify(dc->snapshot(),ichan,intensity_threshold,area_threshold))
            tiling->markActive();
        }
        tiling->fillHolesInActive();
        tiling->dilateActive();
        return 1;
      Error:
        return 0;
      }

      unsigned int AutoTileAcquisition::run(device::Microscope *dc)
      { unsigned eflag=0; //success
        CHKJMP(explore(dc));
        
        { TiledAcquisition t;
          CHKJMP(0==t.run(dc));
        }
        
        { Cut c;
          CHKJMP(0==c.run(dc));
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
