/*
 * File.cpp
 *
 *  Created on: Apr 23, 2010
 *      Author: Nathan Clack <clackn@janelia.hhmi.org>
 */
/*
 * Copyright 2010 Howard Hughes Medical Institute.
 * All rights reserved.
 * Use is subject to Janelia Farm Research Campus Software Copyright 1.1
 * license terms (http://license.janelia.org/license/jfrc_copyright_1_1.html).
 */

#include "util/util-mylib.h"
#include "util/native-buffered-stream.h"
#include <string>
#include <sstream>
#include "devices/DiskStream.h"
#include "frame.h"
#include "File.h"
#include "util/util-file.h"
#include "util/timestream.h"

//#define DEBUG
#undef DEBUG

#define PROFILE
#ifdef PROFILE // PROFILING
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
#define disk_stream_debug(...) debug(__VA_ARGS__)
#else
#define disk_stream_debug(...)
#endif


using namespace mylib;

namespace fetch {
namespace task {
namespace file {


  // Upcasting
  unsigned int ReadRaw::config      (IDevice *d) {return config(dynamic_cast<device::HFILEDiskStreamBase*>(d));}
  unsigned int ReadRaw::run         (IDevice *d) {return run   (dynamic_cast<device::HFILEDiskStreamBase*>(d));}

  unsigned int WriteRaw::config     (IDevice *d) {return config(dynamic_cast<device::HFILEDiskStreamBase*>(d));}
  unsigned int WriteRaw::run        (IDevice *d) {return run   (dynamic_cast<device::HFILEDiskStreamBase*>(d));}

  unsigned int ReadMessage::config  (IDevice *d) {return config(dynamic_cast<device::HFILEDiskStreamBase*>(d));}
  unsigned int ReadMessage::run     (IDevice *d) {return run   (dynamic_cast<device::HFILEDiskStreamBase*>(d));}

  unsigned int WriteMessage::config (IDevice *d) {return config(dynamic_cast<device::HFILEDiskStreamBase*>(d));}
  unsigned int WriteMessage::run    (IDevice *d) {return run   (dynamic_cast<device::HFILEDiskStreamBase*>(d));}

  unsigned int WriteMessageAsRaw::config (IDevice *d) {return config(dynamic_cast<device::HFILEDiskStreamBase*>(d));}
  unsigned int WriteMessageAsRaw::run    (IDevice *d) {return run   (dynamic_cast<device::HFILEDiskStreamBase*>(d));}

  unsigned int TiffStreamReadTask::config (IDevice *d) {return config(dynamic_cast<device::TiffStream*>(d));}
  unsigned int TiffStreamReadTask::run    (IDevice *d) {return run   (dynamic_cast<device::TiffStream*>(d));}

  unsigned int TiffStreamWriteTask::config (IDevice *d) {return config(dynamic_cast<device::TiffStream*>(d));}
  unsigned int TiffStreamWriteTask::run    (IDevice *d) {return run   (dynamic_cast<device::TiffStream*>(d));}

  unsigned int TiffGroupStreamReadTask::config (IDevice *d) {return config(dynamic_cast<device::TiffGroupStream*>(d));}
  unsigned int TiffGroupStreamReadTask::run    (IDevice *d) {return run   (dynamic_cast<device::TiffGroupStream*>(d));}

  unsigned int TiffGroupStreamWriteTask::config (IDevice *d) {return config(dynamic_cast<device::TiffGroupStream*>(d));}
  unsigned int TiffGroupStreamWriteTask::run    (IDevice *d) {return run   (dynamic_cast<device::TiffGroupStream*>(d));}


  //
  // Implementation
  //

  namespace internal
  {
    template<typename T>
    inline T min(const T& a, const T& b) {return (a<b)?a:b;}
    template<typename T>
    inline T max(const T& a, const T& b) {return (a<b)?b:a;}
  }

  /////////////////////////////////////////////////////////////////////////////
  //  READRAW  ////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////
  unsigned int
  ReadRaw::config(device::HFILEDiskStreamBase *dc)
  {return 1;}

  unsigned int
  ReadRaw::run(device::HFILEDiskStreamBase *dc)
  { Chan *q  = dc->_out->contents[0],
         *writer;
    void *buf = Chan_Token_Buffer_Alloc(q);
    DWORD nbytes = Chan_Buffer_Size_Bytes(q),
          bytes_read;
    TicTocTimer t = tic();
    const char* filename = dc->get_config().path().c_str();
    writer=Chan_Open(q,CHAN_WRITE);
    do
    { double dt;
      Guarded_Assert_WinErr(
        ReadFile( dc->_hfile, buf, nbytes, &bytes_read, NULL ));
      Guarded_Assert(CHAN_SUCCESS( Chan_Next(writer,&buf,nbytes) ));
      dt = toc(&t);
      debug("Read %s bytes: %d\r\n"
            "\t%-7.1f bytes per second (dt: %f)\r\n",
            filename, nbytes, nbytes/dt, dt );
    } while ( nbytes && !dc->_agent->is_stopping() );
    Chan_Close(writer);
    Chan_Token_Buffer_Free(buf);
    return 0; // success
  }

  /////////////////////////////////////////////////////////////////////////////
  //  WRITERAW  ///////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  unsigned int
  WriteRaw::config(device::HFILEDiskStreamBase *dc)
  {return 1;}

  unsigned int
  WriteRaw::run(device::HFILEDiskStreamBase *dc)
  { Chan *q  = Chan_Open(dc->_in->contents[0],CHAN_READ);
    void *buf = Chan_Token_Buffer_Alloc(q);
    DWORD nbytes = Chan_Buffer_Size_Bytes(q),
          written;

    TicTocTimer t = tic();

    while( CHAN_SUCCESS(Chan_Next(q,&buf,nbytes)) ) //!dc->_agent->is_stopping() &&
      { double dt = toc(&t);
        nbytes = (DWORD) Chan_Buffer_Size_Bytes(q);
        disk_stream_debug("FPS: %3.1f Frame time: %5.4f            MB/s: %3.1f Q: %3d Write %8d bytes to %s\r\n",
                1.0/dt, dt,                      nbytes/1000000.0/dt,
                q->q->head - q->q->tail,nbytes, stream->path );
        Guarded_Assert_WinErr( WriteFile( dc->_hfile, buf, nbytes, &written, NULL ));
        Guarded_Assert( written == nbytes );
      }
    Chan_Close(q);
    Chan_Token_Buffer_Free(buf);
    return 0; // success
  }

  /////////////////////////////////////////////////////////////////////////////
  //  READMESSAGE  ////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////
/*
 * [ ] rewrite read to use message to/from file
 * [ ] rewrite write to use message to/from file
 *
 */
  unsigned int
  ReadMessage::config(device::HFILEDiskStreamBase *dc)
  {return 1;}

  unsigned int
  ReadMessage::run(device::HFILEDiskStreamBase *dc)
  { Chan   *q   = Chan_Open(dc->_out->contents[0],CHAN_WRITE);
    Message *buf = NULL;
    DWORD    nbytes;
    i64      sz,
             maxsize;
    const char* filename = dc->get_config().path().c_str();

    // Scan file to find max buffer size required
    { maxsize=0;
      sz = 0;
      TicTocTimer t = tic();
      while (w32file::setpos(dc->_hfile, sz, FILE_CURRENT)>=0) // jump to next message
      { sz = Message::from_file(dc->_hfile, NULL, 0);          // get size of this message
        maxsize = internal::max( maxsize, sz );                          // update the max size
      }
      w32file::setpos(dc->_hfile,0,FILE_BEGIN);                // rewind
      debug("Max buffer size of %lld found in %f seconds\r\n",maxsize,toc(&t));
    }
    Chan_Resize(q,(size_t)maxsize);                     // Make sure the queue's sized right
    buf = (Message*)Chan_Token_Buffer_Alloc(q);                 // get the first container

    // Producer loop
    TicTocTimer t = tic();
    FrmFmt eg;
    nbytes = (DWORD) maxsize;
    while ( nbytes && !dc->_agent->is_stopping() && !w32file::eof(dc->_hfile) )
    { double dt;
      Guarded_Assert( Message::from_file(dc->_hfile,NULL,0));                                // get size
      Guarded_Assert( Message::from_file(dc->_hfile,buf,nbytes)==0);                         // read data
      // FIXME:
      // a) it looks like I inadvertently serialize the virtual table pointers for each Message
      // b) should rebuild the virtual table for each type before sending it out?  Any virtual
      //    methods are invalid otherwise.
      // I think the conclusion here is that this was a bad approach.  Should use a "real"
      // format.  That is, ideally, tiff.  This should still work for limited serialization
      // of Messages so I won't take it out yet.  Maybe I can still convert data I've already
      // taken.
      buf->cast(); // dark magic - casts to specific message class corresponding to buf->id.
      Guarded_Assert(CHAN_SUCCESS( Chan_Next(q,(void**)&buf,nbytes) ));  // push
      dt = toc(&t);
      debug("Read %s bytes: %d\r\n"
        "\t%-7.1f bytes per second (dt: %f)\r\n",
        filename, nbytes, nbytes/dt, dt );
    }
    Chan_Close(q);
    Chan_Token_Buffer_Free(buf);
    return 0; // success
  }

  /////////////////////////////////////////////////////////////////////////////
  //  WRITEMESSAGE  ///////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////
  unsigned int
  WriteMessage::config(device::HFILEDiskStreamBase *dc)
  {return 1;}

  unsigned int
  WriteMessage::run(device::HFILEDiskStreamBase *dc)
  { Chan *q   = Chan_Open(dc->_in->contents[0],CHAN_READ);
    void *buf = Chan_Token_Buffer_Alloc(q);
    size_t nbytes = Chan_Buffer_Size_Bytes(q);

    TicTocTimer t = tic();
    while(CHAN_SUCCESS( Chan_Next(q,&buf,nbytes) ))                 //!dc->_agent->is_stopping() &&
      { double dt = toc(&t);
        nbytes = ((Message*)buf)->size_bytes();
        disk_stream_debug("FPS: %3.1f Frame time: %5.4f            MB/s: %3.1f Q: %3d Write %8d bytes to %s\r\n",
                1.0/dt, dt,                      nbytes/1000000.0/dt,
                q->q->head - q->q->tail,nbytes, dc->path );
        ((Message*)buf)->to_file(dc->_hfile);
      }
    Chan_Close(q);
    Chan_Token_Buffer_Free(buf);
    return 0; // success
  }

  unsigned int
  WriteMessageAsRaw::config(device::HFILEDiskStreamBase *dc)
  {return 1;}

  unsigned int
  WriteMessageAsRaw::run(device::HFILEDiskStreamBase *dc)
  { Chan *q  = Chan_Open(dc->_in->contents[0],CHAN_READ);
    Message *buf = (Message*) Chan_Token_Buffer_Alloc(q);
    DWORD nbytes = Chan_Buffer_Size_Bytes(q);
    DWORD written;

    TicTocTimer t = tic();
    while(CHAN_SUCCESS( Chan_Next(q,(void**)&buf,nbytes) ))        //!dc->_agent->is_stopping() &&
      { double dt = toc(&t);
        nbytes = buf->size_bytes() - buf->self_size;
        disk_stream_debug("FPS: %3.1f Frame time: %5.4f            MB/s: %3.1f Q: %3d Write %8d bytes to %s\r\n",
                1.0/dt, dt,                      nbytes/1000000.0/dt,
                q->q->head - q->q->tail,nbytes, dc->path );
        Guarded_Assert_WinErr( WriteFile( dc->_hfile, buf->data, nbytes, &written, NULL ));
        Guarded_Assert( written == nbytes );
      }
    Chan_Token_Buffer_Free(buf);
    return 0; // success
  }

  /////////////////////////////////////////////////////////////////////////////
  // TIFF STREAM READER  //////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  unsigned int TiffStreamReadTask::config( device::TiffStream *dc )
  {
    return 1;
  }

  unsigned int TiffStreamReadTask::run( device::TiffStream *dc )
  {
    Chan *q = Chan_Open(dc->_out->contents[0],CHAN_WRITE);
    Tiff_IFD *ifd;
    Tiff_Image *tim;
    Tiff_Reader *tif = dc->_tiff_reader;
    Frame_With_Interleaved_Planes *buf = NULL;
    size_t maxsize = 0;
    int    maxchan = 0;
    u8 **planes = NULL;
    int eflag=0;

    // compute max image size
    {

      while(!End_Of_Tiff(dc->_tiff_reader))
      {
        goto_if_fail(ifd=Read_Tiff_IFD(tif),FailedIFDRead);
        goto_if_fail(tim=Get_Tiff_Image(ifd),FailedImageGet);
        Frame_With_Interleaved_Planes fmt(tim->width,tim->height,tim->number_channels,mytiff::pixel_type(tim));
        mylib::Free_Tiff_Image(tim);
        mylib::Free_Tiff_IFD(ifd);
        maxsize = internal::max(maxsize,fmt.size_bytes());
        maxchan = internal::max(maxchan,tim->number_channels);
      }
      mylib::Rewind_Tiff_Reader(tif);

    }

    // Resize Queue if necessary
    Chan_Resize(q,maxsize);                                   // Make sure the queue's sized right
    buf = (Frame_With_Interleaved_Planes*)Chan_Token_Buffer_Alloc(q); // get the first container

    // Producer loop
    planes = (u8**)malloc(sizeof(u8*)*maxchan);
    size_t nbytes = maxsize;
    size_t pitches[4];
    while (nbytes && !dc->_agent->is_stopping() && !End_Of_Tiff(tif))
    {

      goto_if_fail(ifd=Read_Tiff_IFD(tif),FailedIFDRead);
      goto_if_fail(tim=Get_Tiff_Image(ifd),FailedImageGet);
      Frame_With_Interleaved_Planes fmt(tim->width,tim->height,tim->number_channels,mytiff::pixel_type(tim));
      nbytes = fmt.size_bytes();
      buf->format(&fmt);
      planes[0] = (u8*)buf->data;
      buf->compute_pitches(pitches);
      for(int i=1;i<tim->number_channels;++i)
        planes[i] = planes[0] + pitches[1];
      Load_Tiff_Image_Planes(tim,(void**)planes); // Copy from disk into buffer
      Free_Tiff_Image(tim);
      Free_Tiff_IFD(ifd);

      goto_if_fail(
        CHAN_SUCCESS(Chan_Next(q,(void**)&buf,nbytes)),
        FailedPush);
    }
Finalize:
    Chan_Token_Buffer_Free(buf);
    Chan_Close(q);
    return eflag; // success
FailedPush:
    if(planes) free(planes);
    error("Failed push while reading Tiff\r\n.");
    eflag=1;
    goto Finalize;
FailedImageGet:
    warning("Failed to interpret IFD (0x%p) from TIFF while streaming.\r\n\tGot: %s\r\n\t In: %s\r\n",ifd,Tiff_Error_String(),Tiff_Error_Source());
    Free_Tiff_IFD(ifd);

    if(planes) free(planes);
    eflag=2;
    goto Finalize;
FailedIFDRead:
    warning("Failed to read IFD from TIFF while streaming.\r\n\tGot: %s\r\n\t In: %s\r\n",Tiff_Error_String(),Tiff_Error_Source());

    if(planes) free(planes);
    eflag=3;
    goto Finalize;
  }

  /////////////////////////////////////////////////////////////////////////////
  //  TIFF STREAM WRITER  /////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////
  unsigned int TiffStreamWriteTask::config( device::TiffStream *dc )
  {
    return 1;
  }

  unsigned int TiffStreamWriteTask::run( device::TiffStream *dc )
  {
    Chan *q  = Chan_Open(dc->_in->contents[0],CHAN_READ);
    Frame_With_Interleaved_Planes *buf = (Frame_With_Interleaved_Planes*)Chan_Token_Buffer_Alloc(q);
    Tiff_Writer *tif = dc->_tiff_writer;
    Tiff_Image *tim;
    Tiff_IFD *ifd;
    size_t nbytes = Chan_Buffer_Size_Bytes(q);
    int eflag;
    TS_OPEN("timer-TiffStreamWrite.f32");

    while(CHAN_SUCCESS( Chan_Next(q,(void**)&buf,nbytes) ))      //!dc->_agent->is_stopping() &&
    { TS_TIC;
      goto_if_fail(buf->id==FRAME_INTERLEAVED_PLANES,FailureBufferHasWrongFormat);
      nbytes = buf->size_bytes();

      //buf->dump("TiffStreamWriteTask-src.%s",TypeStrFromID(buf->rtti));
      //buf->totif("TiffStreamWriteTask-src.tif");

      {
        goto_if_fail(tim=Create_Tiff_Image(buf->width,buf->height),FailedCreateTIFFImage);

        int scale = g_type_attributes[buf->rtti].bits;
        Channel_Type type = CHAN_UNSIGNED;
        if(TYPE_IS_FLOATING(buf->rtti))
          type = CHAN_FLOAT;
        else if(TYPE_IS_SIGNED(buf->rtti))
          type = CHAN_SIGNED;

        size_t pitches[4], pp; //pp is the plane pitch in bytes
        buf->compute_pitches(pitches);
        pp = pitches[1];
        u8 *data = (u8*) buf->data;
        for(int i=0;i<buf->nchan;++i)
          goto_if(Add_Tiff_Image_Channel(tim,CHAN_BLACK,scale,type,data+i*pp),FailedAddChannel);
        goto_if_fail(ifd=Make_IFD_For_Image(tim,DONT_COMPRESS,0,0),FailedMakeIFD);
        goto_if_fail(Write_Tiff_IFD(tif,ifd)==0,FailedWriteIFD);
        TS_TOC;
      }

    }
    eflag=0; //success
Finalize:
    TS_CLOSE;
    Chan_Close(q);
    Chan_Token_Buffer_Free(buf);
    return eflag;
FailureBufferHasWrongFormat:
    error("Received a buffer from the input queue with an unexpected format. (wanted FRAME_INTERLEAVED_PLANES)\r\n\tGot: %d\r\n",buf->id);
    eflag=1;
    goto Finalize;
FailedCreateTIFFImage:
    error("Failed to create TIFF image while streaming.\r\n\tGot: %s\r\n\t In: %s\r\n",Tiff_Error_String(),Tiff_Error_Source());

    eflag=2;
    goto Finalize;
FailedAddChannel:
    error("Adding a channel to a tiff image (0x%p) failed.\r\n\tGot: %s\r\n\t In: %s\r\n",tim,Tiff_Error_String(),Tiff_Error_Source());
    Free_Tiff_Image(tim);

    eflag=3;
    goto Finalize;
FailedMakeIFD:
    error("Failed to make Tiff IFD for tiff image (0x%p).\r\n\tGot: %s\r\n\t In: %s\r\n",tim,Tiff_Error_String(),Tiff_Error_Source());
    Free_Tiff_Image(tim);

    eflag=4;
    goto Finalize;
FailedWriteIFD:
    error("Write Tiff IFD failed. Reader: 0x%p IFD: 0x%p.\r\n\tGot: %s\r\n\t In: %s\r\n",tim,ifd,Tiff_Error_String(),Tiff_Error_Source());
    Free_Tiff_IFD(ifd);
    Free_Tiff_Image(tim);

    eflag=5;
    goto Finalize;
  }

  /////////////////////////////////////////////////////////////////////////////
  //  TIFF GROUP STREAM READER  ///////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////
  unsigned int TiffGroupStreamReadTask::config(device::TiffGroupStream *dc)
  { return 1; }

  unsigned int TiffGroupStreamReadTask::run( device::TiffGroupStream *dc )
  { warning("%s(%d): Not Implemented\r\n",__FILE__,__LINE__);
    return 0;
  }

  /////////////////////////////////////////////////////////////////////////////
  //  TIFF GROUP STREAM WRITER  ///////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  /** \class TiffGroupStreamWriteTask
      Appends the colors from each image to corresponding tiff stacks.

      One stack per color.

      The number of channels is required to know exactly how many tiff writers
      to open, so the writers aren't opened till the task is started.

      That means:
      1. Don't implicitly have validation of path before run().
      2. Pausing and restarting an acquisition to disk will cause the files to get
         overwritten rather than appended to.

         Could address this.
         a. Resize as necessary in the run thread.
            Do not clean up writers on thread exit ("stop").
         b. Clean up writers on detach()
         c. When re-running, just append to old writers.

      What to do when number of channels or image size changes?
      - I think the behavior of the TiffStreamWriteTask is to append images of different size
      - This behavior doesn't occur during normal operation

      \todo what if the number of channels changes
  */

#define ENDL "\r\n"
#define TRY(e)     do{ DBG(#e); if(!(e)) {warning("%s(%d):"ENDL "\t%s"ENDL "\tExpression evaluated to false."ENDL,__FILE__,__LINE__,#e); goto Error;} }while(0)
#define TIFFTRY(e) do{ DBG(#e); if(!(e)) {warning("%s(%d):"ENDL "\t%s"ENDL "\t[TIFF] Expression evaluated to false."ENDL "%s",__FILE__,__LINE__,#e,(const char*)Image_Error()); Image_Error_Release(); goto Error;} }while(0)
#define TODO       error("%s(%d): [TODO] Not Implemented\r\n",__FILE__,__LINE__)
#if 0
#define DBG(msg)   debug("%s(%d): %s"ENDL "\t%s"ENDL,__FILE__,__LINE__,__FUNCTION__,msg)
#else
#define DBG(msg)
#endif
#define countof(e) (sizeof(e)/sizeof(*(e)))

  // Allocate storage for the native buffered stream from a shallow pool of
  //.buffers.  Will block none are available
  static struct bufs_t_
  { void *p;
    size_t sz;
    unsigned           inuse;
    CONDITION_VARIABLE available;
  } bufs_[8]={0};  // want two per channel

  static SRWLOCK bufs_lock_={0};
  static int     ibufs_=0;

  static void lock()   {AcquireSRWLockExclusive(&bufs_lock_);}
  static void unlock() {ReleaseSRWLockExclusive(&bufs_lock_);}

  static void* bufs_alloc__inlock(size_t nbytes)
  { void *p=0;
    struct bufs_t_ *b=bufs_+ibufs_;
    ibufs_= (ibufs_+1)%countof(bufs_);
    DBG("TiffGroupStreamWriter: Buffer %5u -- WAIT"ENDL, (unsigned) (ibufs_));
    while(b->inuse)
      TRY(SleepConditionVariableSRW(&b->available,&bufs_lock_,INFINITE,0));
    DBG("TiffGroupStreamWriter: Buffer %5u -- OPEN"ENDL, (unsigned) (ibufs_));
    if(b->sz<nbytes)
    { TRY(b->p=realloc(b->p,b->sz=nbytes));
    }
    p=b->p;
    b->inuse=1;
  Error:
    return p;
  }

  static int bufs_wait()
  { int isok=0;
    lock();
    struct bufs_t_ *b=bufs_+ibufs_;
    while(b->inuse)
      TRY(SleepConditionVariableSRW(&b->available,&bufs_lock_,INFINITE,0));
    isok=1;
  Error:
    unlock();
    return isok;
  }

  static void* bufs_alloc(size_t nbytes)
  { void *p=0;
    lock();
    p=bufs_alloc__inlock(nbytes);
    unlock();
    return p;
  }

  static void bufs_free(void *p)
  { struct bufs_t_ *b=bufs_+countof(bufs_);
    lock();
    while(b-->bufs_ && p!=b->p);
    TRY(b->p==p);
    DBG("TiffGroupStreamWriter: Buffer %5u -- RELEASE"ENDL, (unsigned) (b-bufs_));
    b->inuse=0;
    WakeConditionVariable(&b->available);
Error:
    unlock();
  }

  static void bufs_print_stats__inlock(void)
  { char a[]="             ",
         u[9]={0};
    a[ibufs_]='^';
    for(int i=0;i<8;++i) u[i]=bufs_[i].inuse?'x':'.';
    debug("[TiffGroupStreamWriteTask] bufs %5d %s\n"
          "                                      %s\n",ibufs_,u,a);
  }


  static void* bufs_realloc(void *p,size_t nbytes)
  { struct bufs_t_ *b=bufs_+countof(bufs_);
    lock();
    bufs_print_stats__inlock();
    if(!p)
      TRY(p=bufs_alloc__inlock(nbytes));
    else
    { while(b-->bufs_ && p!=b->p);
      TRY(b->p==p);
      TRY(b->inuse==1); // sanity check
      if(b->sz<nbytes)
      { nbytes = 4096*( (unsigned)((1.5f*nbytes+4096.0f)/4096)); // geometric size increase aligned to 4096 bytes
        TRY(p=b->p=realloc(b->p,b->sz=nbytes));
      }
    }
Finalize:
    unlock();
    return p;
Error:
    p=NULL;
    goto Finalize;
  }

  /** \todo Find out (and fix) what happens when the root name lacks an extension */
  static ::std::string gen_name(const ::std::string & root, int i)
  { ::std::ostringstream os;
    size_t idot = root.rfind('.');
    std::string firstpart  = root.substr(0,idot),
                secondpart = root.substr(idot,std::string::npos);
    os << firstpart << "." << i << secondpart; // eg default.0.tif
    return os.str();
  }

  unsigned int TiffGroupStreamWriteTask::config(device::TiffGroupStream *dc)
  {
    //std::vector<mylib::stream_t>   streams;
    TRY(dc->nchan()>0);
    for(int i=0;i<dc->nchan();++i)
    {
      if(i>=dc->_writers.size())
      { device::TiffGroupStream::Config c = dc->get_config();
        TicTocTimer t;
        ::std::string fname = gen_name(c.path(),i);
        mylib::Tiff* tif=0;
        mylib::stream_t s=0;
        t=tic();
  #if 0
  #define TIME(e) do{ TicTocTimer t=tic(); e; debug("[TIME] %10.6f msec\t%s\n",1000.0*toc(&t),#e); }while(0)
  #else
  #define TIME(e) e
  #endif
        TIME( TRY(s=native_buffered_stream_open(fname.c_str(),STREAM_MODE_WRITE)));
        TIME( native_buffered_stream_set_malloc_func (s,bufs_alloc));
        TIME( native_buffered_stream_set_realloc_func(s,bufs_realloc));
        TIME( native_buffered_stream_set_free_func   (s,bufs_free));
        TIME( TRY(native_buffered_stream_reserve(s,256*1024*1024))); // one stack's worth per channel
        TIME( TIFFTRY(tif=Open_Tiff_Stream(s,"w")));
  #undef TIME
        debug("Tiff Stream Open: %f msec\r\n",toc(&t)*1.0e3);
        //streams.push_back(s);
        dc->_writers.push_back(tif);
      }
    }

    return bufs_wait()==1;
  Error:
    return 0;
  }  // will block caller until the next buffer is available


#define DEBUG_FAST_EXIT
#include <util/native-buffered-stream.h>
  unsigned int TiffGroupStreamWriteTask::run(device::TiffGroupStream *dc)
  { int                            ecode=0;
    Chan                          *q  =0;
    Frame_With_Interleaved_Planes *buf=0;
    size_t                         nbytes;
    std::vector<mylib::stream_t>   streams;
    int                            i;
    TS_OPEN("timer-TiffGroupStreamWrite.f32");
#ifdef DEBUG_FAST_EXIT
    int any=0;
#endif
    TRY(q=Chan_Open(dc->_in->contents[0],CHAN_READ));
    TRY(buf=(Frame_With_Interleaved_Planes*)Chan_Token_Buffer_Alloc(q));
    nbytes=Chan_Buffer_Size_Bytes(q);

    DBG("Entering Loop");
    //int count=0;
    while(CHAN_SUCCESS(Chan_Next(q,(void**)&buf,nbytes)))
    { Array     dummy;
      Dimn_Type dims[3];
      DBG("Recieved");
      //debug("TiffGroupStream recieved frame %d\n",count++);
#ifdef DEBUG_FAST_EXIT
      any=1;
#endif
      TS_TIC;
      TRY(buf->id==FRAME_INTERLEAVED_PLANES);
      mylib::castFetchFrameToDummyArray(&dummy,buf,dims);

      for(i=0;i<dims[2];++i)
      {
#if 0
        // maybe append writer
        if(i>=dc->_writers.size())
        { device::TiffGroupStream::Config c = dc->get_config();
          TicTocTimer t;
          ::std::string fname = gen_name(c.path(),i);
          mylib::Tiff* tif=0;
          mylib::stream_t s=0;
          t=tic();
#if 0
#define TIME(e) do{ TicTocTimer t=tic(); e; debug("[TIME] %10.6f msec\t%s\n",1000.0*toc(&t),#e); }while(0)
#else
#define TIME(e) e
#endif
          TIME( TRY(s=native_buffered_stream_open(fname.c_str(),STREAM_MODE_WRITE))); /// FIXME: The CreateFile call can take a long time. --> depends on filename
          TIME( native_buffered_stream_set_malloc_func (s,bufs_alloc));
          TIME( native_buffered_stream_set_realloc_func(s,bufs_realloc));
          TIME( native_buffered_stream_set_free_func   (s,bufs_free));
          TIME( TRY(native_buffered_stream_reserve(s,256*1024*1024))); // one stack's worth per channel
          TIME( TIFFTRY(tif=Open_Tiff_Stream(s,"w")));
#undef TIME
          debug("Tiff Stream Open: %f msec\r\n",toc(&t)*1.0e3);
          streams.push_back(s);
          dc->_writers.push_back(tif);
        }
#endif
        // Write out channel
        { mylib::Tiff* w = dc->_writers[i];
          Array_Bundle tmp = dummy;
          TIFFTRY(0==Add_IFD_Channel(w,Get_Array_Plane(&tmp,i),PLAIN_CHAN));
          Update_Tiff(w,DONT_PRESS);
        }
      }
      TS_TOC;
    }

    for(i=0;i<dc->_writers.size();++i)
      if(dc->_writers[i])
        mylib::Close_Tiff(dc->_writers[i]);
    dc->_writers.clear();
Finalize:
    DBG("Done.");
    TS_CLOSE;
#ifdef DEBUG_FAST_EXIT
    if(!any)
      DBG("NEVER POPPED");
#endif
    if(q)   Chan_Close(q);
    if(buf) Chan_Token_Buffer_Free(buf);
    return ecode;
Error:
    ecode = 1;
    goto Finalize;
  }

}  // namespace file
}  // namespace task
}  // namespace fetch


