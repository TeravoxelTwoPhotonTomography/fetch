/*
 * SurfaceFindWorker.cpp
 *
 *  Created on: Apr 22, 2010
 *      Author: Nathan Clack <clackn@janelia.hhmi.org>
 */
/*
 * Notes for change to resizable queue
 * -----------------------------------
 * - need current buffer size for push/pop...just need to keep track
 * - what happens if the source buffer changes in the middle of an average
 */
#include "config.h"
#include "SurfaceFindWorker.h"
#include "util/util-mylib.h"

//#define PROFILE
#if 0 //def PROFILE // PROFILING
#define TS_OPEN(...)    timestream_t ts__=timestream_open(__VA_ARGS__)
#define TS_TIC          timestream_tic(ts__)
#define TS_TOC          timestream_toc(ts__)
#define TS_CLOSE        timestream_close(ts__)
#else
#define TS_OPEN(...)
#define TS_TIC
#define TS_TOC
#define TS_CLOSE
#endif

#if 0
#define ECHO(estr)   LOG("---\t%s\n",estr)
#else
#define ECHO(estr)
#endif
#if 0
#define DBG(...) debug(__VA_ARGS__)
#else
#define DBG(...)
#endif

#define LOG(...)     DBG(__VA_ARGS__)
#define REPORT(estr) LOG("%s(%d): %s()\n\t%s\n\tEvaluated to false.\n",__FILE__,__LINE__,__FUNCTION__,estr)
#define TRY(e)       do{ECHO(#e);if(!(e)){REPORT(#e);goto Error;}}while(0)

#define REMIND(expr) \
  warning("%s(%d): %s"ENDL "\tDumping debug data."ENDL, __FILE__,__LINE__,__FUNCTION__);\
  (expr)

using namespace fetch::worker;

namespace fetch
{
  bool operator==(const cfg::tasks::SurfaceFind& a, const cfg::tasks::SurfaceFind& b)
  { return  (a.ichan()==b.ichan()) &&
            (a.intensity_threshold()==b.intensity_threshold()) &&
            (a.area_threshold()==b.area_threshold());
  }
  bool operator!=(const cfg::tasks::SurfaceFind& a, const cfg::tasks::SurfaceFind& b)
  { return !(a==b);
  }
  namespace task
  {


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
        DBG("Fraction above thresh: %f",count/((double)image->size));
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


    unsigned int
    SurfaceFindWorker::run(IDevice *idc)
    { int eflag = 0;
      SurfaceFindWorkerAgent *dc = dynamic_cast<SurfaceFindWorkerAgent*>(idc);
      TS_OPEN("timer-SurfaceFindWorker.f32");

      // open channels
      Chan *qsrc = dc->_in->contents[0],
           *qdst = dc->_out->contents[0],
           *reader, *writer;

      Frame_With_Interleaved_Planes  *fsrc =  (Frame_With_Interleaved_Planes*)Chan_Token_Buffer_Alloc(qsrc);
      size_t nbytes_in  = Chan_Buffer_Size_Bytes(qsrc);
      reader = Chan_Open(qsrc,CHAN_READ);
      writer = Chan_Open(qdst,CHAN_WRITE);

      // MAIN LOOP
      dc->reset();
      size_t count=0;
      while(CHAN_SUCCESS(Chan_Next(reader,(void**)&fsrc,nbytes_in)))
      { nbytes_in = fsrc->size_bytes();
        mylib::Array im;
        mylib::Dimn_Type dims[3]={0};

        //REMIND(fsrc->totif("SurfaceFindWorker-src.tif"));
        TS_TIC;          
        mylib::castFetchFrameToDummyArray(&im,fsrc,dims);
        if(classify(&im,dc->get_config().ichan(),dc->get_config().intensity_threshold(),dc->get_config().area_threshold()))
        { LOG("[SurfaceFindWorker] Classify() triggered on count %d\n",count);
          dc->set(count);         
        }
        ++count;
        TS_TOC;
        //REMIND(fdst->totif("SurfaceFindWorker-dst.tif"));
        
        TRY(CHAN_SUCCESS(Chan_Next(writer,(void**)&fsrc,fsrc->size_bytes())));        
      }
Finalize:
      TS_CLOSE;
      Chan_Close(reader);
      Chan_Close(writer);
      Chan_Token_Buffer_Free(fsrc);
      return eflag;
Error:
      warning("%s(%d) %s()\r\n\tSomething went wrong with the SurfaceFindWorker.\r\n",__FILE__,__LINE__,__FUNCTION__);
      eflag=1;
      goto Finalize;
    }

  } // fetch::task

  namespace worker
  {
    SurfaceFindWorkerAgent::SurfaceFindWorkerAgent(): WorkAgent<TaskType,Config>("SurfaceFindWorker")
      ,last_found_(0)
      ,any_found_(0)
    {}

    SurfaceFindWorkerAgent::SurfaceFindWorkerAgent(Config *config): WorkAgent<TaskType,Config>(config,"SurfaceFindWorker")
      ,last_found_(0)
      ,any_found_(0)
    {}

    void     SurfaceFindWorkerAgent::set(unsigned i)     {if(!any_found_) {any_found_=1; last_found_=i;}}
    unsigned SurfaceFindWorkerAgent::which()             {return last_found_;}
    unsigned SurfaceFindWorkerAgent::any()               {return !(too_inside()||too_outside());}
    void     SurfaceFindWorkerAgent::reset()             {any_found_=0;last_found_=0;}
    unsigned SurfaceFindWorkerAgent::too_inside()        { return any_found_ && (which()<=1); }
    unsigned SurfaceFindWorkerAgent::too_outside()       { return !any_found_; }

  } //fetch::worker
}   // fetch