/*
 * Pipeline.cpp
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
#include "Pipeline.h"
#include "algo/pipeline.h"
#include "algo/pipeline-image-frame.h"

//#define PROFILE
#if 1 //def PROFILE // PROFILING
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
  bool operator==(const cfg::worker::Pipeline& a, const cfg::worker::Pipeline& b)
  { return  (a.frame_average_count()==b.frame_average_count()) &&
            (a.downsample_count()==b.downsample_count()) &&
            (a.invert_intensity()==b.invert_intensity());
  }
  bool operator!=(const cfg::worker::Pipeline& a, const cfg::worker::Pipeline& b)
  { return !(a==b);
  }
  namespace task
  {
    unsigned int
    Pipeline::run(IDevice *idc)
    { int eflag = 0;
      PipelineAgent *dc = dynamic_cast<PipelineAgent*>(idc);
      pipeline_param_t params={0};
      pipeline_t ctx=0;
      TS_OPEN("timer-pipeline.f32");

      // read in parameters
      { const cfg::worker::Pipeline cfg=dc->get_config();
        params.frame_average_count = cfg.frame_average_count();
        params.pixel_average_count = cfg.downsample_count();
        params.invert_intensity    = (unsigned) cfg.invert_intensity();
        params.scan_rate_Hz        = dc->scan_rate_Hz();
        params.sample_rate_MHz     = dc->sample_rate_Mhz();
      }
      TRY(ctx=pipeline_make(&params));

      // open channels
      pipeline_image_t pipesrc=0,pipedst=0;
      Chan *qsrc = dc->_in->contents[0],
           *qdst = dc->_out->contents[0],
           *reader, *writer;

      Frame_With_Interleaved_Planes *fsrc = (Frame_With_Interleaved_Planes*) Chan_Token_Buffer_Alloc(qsrc),
                                    *fdst = (Frame_With_Interleaved_Planes*) Chan_Token_Buffer_Alloc(qdst);
      f32 *acc = NULL;
      { // init fdst
        size_t dst_bytes = Chan_Buffer_Size_Bytes(qdst);
        Frame_With_Interleaved_Planes ref(dst_bytes,1,1,id_u8); // just a 1d array with the right number of bytes. dst will get formated correctly later.
        ref.format(fdst);
      }
      reader = Chan_Open(qsrc,CHAN_READ);
      writer = Chan_Open(qdst,CHAN_WRITE);

      // MAIN LOOP
      size_t src_bytes=Chan_Buffer_Size_Bytes(qsrc);
      while(CHAN_SUCCESS(Chan_Next(reader,(void**)&fsrc,src_bytes)))
      { int emit=0;
        //REMIND(fsrc->totif("pipeline-src.tif"));
        TS_TIC;
        TRY(pipesrc=pipeline_set_image_from_frame(pipesrc,fsrc));
        TRY(pipedst=pipeline_make_dst_image(pipedst,ctx,pipesrc));
        TRY(fdst=pipeline_format_frame(pipedst,fdst)); // maybe realloc fdst and format the frame.
        pipeline_image_set_data(pipedst,fdst->data);
        TRY(pipeline_exec(ctx,pipedst,pipesrc,&emit));
        TS_TOC;
        if(emit)
        { //REMIND(fdst->totif("pipeline-dst.tif"));
          TRY(CHAN_SUCCESS(Chan_Next(writer,(void**)&fdst,fdst->size_bytes())));
          { // init fdst
            size_t dst_bytes = Chan_Buffer_Size_Bytes(qdst);
            Frame_With_Interleaved_Planes ref(dst_bytes,1,1,id_u8); // just a 1d array with the right number of bytes. dst will get formated correctly later.
            ref.format(fdst);
          }
        }
      }
Finalize:
      TS_CLOSE;
      pipeline_free(&ctx);
      pipeline_free_image(&pipesrc);
      pipeline_free_image(&pipedst);
      Chan_Close(reader);
      Chan_Close(writer);
      Chan_Token_Buffer_Free(fsrc);
      Chan_Token_Buffer_Free(fdst);
      return eflag;
Error:
      warning("%s(%d) %s()\r\n\tSomething went wrong with the pipeline.\r\n",__FILE__,__LINE__,__FUNCTION__);
      eflag=1;
      goto Finalize;
    }

  } // fetch::task

  namespace worker
  {
    PipelineAgent::PipelineAgent(): WorkAgent<TaskType,Config>("Pipeline")
      ,scan_rate_Hz_(7920)
      ,sample_rate_Mhz_(125)
    {}

    PipelineAgent::PipelineAgent(Config *config): WorkAgent<TaskType,Config>(config,"Pipeline")
      ,scan_rate_Hz_(7920)
      ,sample_rate_Mhz_(125)
    {}

    unsigned PipelineAgent::scan_rate_Hz()                  {return scan_rate_Hz_;}
    unsigned PipelineAgent::sample_rate_Mhz()               {return sample_rate_Mhz_;}
    void     PipelineAgent::set_scan_rate_Hz(unsigned v)    {scan_rate_Hz_=v;}
    void     PipelineAgent::set_sample_rate_MHz(unsigned v) {sample_rate_Mhz_=v;}

  } //fetch::worker
}   // fetch