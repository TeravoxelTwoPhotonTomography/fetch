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
#include "stdafx.h"
#include "devices/DiskStream.h"
#include "frame.h"
#include "File.h"
#include "util/util-file.h"
#if 0
#define disk_stream_debug(...) debug(__VA_ARGS__)
#else
#define disk_stream_debug(...)
#endif
#include "MY_TIFF/tiff.image.h"
#include "util/util-mylib.h"

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

  unsigned int TiffStreamReadTask::config (IDevice *d) {return config(dynamic_cast<device::HFILEDiskStreamBase*>(d));}
  unsigned int TiffStreamReadTask::run    (IDevice *d) {return run   (dynamic_cast<device::HFILEDiskStreamBase*>(d));}

  unsigned int TiffStreamWriteTask::config (IDevice *d) {return config(dynamic_cast<device::TiffStream*>(d));}
  unsigned int TiffStreamWriteTask::run    (IDevice *d) {return run   (dynamic_cast<device::TiffStream*>(d));}

 

  //
  // Implementation
  //
  
  unsigned int
  ReadRaw::config(device::HFILEDiskStreamBase *dc)
  {return 1;}

  unsigned int
  ReadRaw::run(device::HFILEDiskStreamBase *dc)
  { asynq *q  = dc->_out->contents[0];
    void *buf = Asynq_Token_Buffer_Alloc(q);
    DWORD nbytes = q->q->buffer_size_bytes,
          bytes_read;
    TicTocTimer t = tic();
    const char* filename = dc->get_config().path().c_str();
    do
    { double dt;
      Guarded_Assert_WinErr(
        ReadFile( dc->_hfile, buf, nbytes, &bytes_read, NULL ));
      Guarded_Assert( Asynq_Push_Timed(q, &buf, nbytes, DISKSTREAM_DEFAULT_TIMEOUT) );
      dt = toc(&t);
      debug("Read %s bytes: %d\r\n"
            "\t%-7.1f bytes per second (dt: %f)\r\n",
            filename, nbytes, nbytes/dt, dt );
    } while ( nbytes && !dc->_agent->is_stopping() );
    Asynq_Token_Buffer_Free(buf);
    return 0; // success
  }



  unsigned int
  WriteRaw::config(device::HFILEDiskStreamBase *dc)
  {return 1;}



  unsigned int
  WriteRaw::run(device::HFILEDiskStreamBase *dc)
  { asynq *q  = dc->_in->contents[0];
    void *buf = Asynq_Token_Buffer_Alloc(q);
    DWORD nbytes = q->q->buffer_size_bytes,
          written;

    TicTocTimer t = tic();
    
    while( !dc->_agent->is_stopping() && Asynq_Pop(q, &buf, nbytes) )
      { double dt = toc(&t);
        nbytes = q->q->buffer_size_bytes;
        disk_stream_debug("FPS: %3.1f Frame time: %5.4f            MB/s: %3.1f Q: %3d Write %8d bytes to %s\r\n",
                1.0/dt, dt,                      nbytes/1000000.0/dt,
                q->q->head - q->q->tail,nbytes, stream->path );
        Guarded_Assert_WinErr( WriteFile( dc->_hfile, buf, nbytes, &written, NULL ));
        Guarded_Assert( written == nbytes );
      }
    
    Asynq_Token_Buffer_Free(buf);
    return 0; // success
  }

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
  { asynq   *q   = dc->_out->contents[0];
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
        maxsize = max( maxsize, sz );                            // update the max size
      } 
      w32file::setpos(dc->_hfile,0,FILE_BEGIN);                // rewind
      debug("Max buffer size of %lld found in %f seconds\r\n",maxsize,toc(&t));  
    }
    Asynq_Resize_Buffers(q,(size_t)maxsize);                     // Make sure the queue's sized right
    buf = (Message*)Asynq_Token_Buffer_Alloc(q);                 // get the first container

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
      Guarded_Assert( Asynq_Push_Timed(q, (void**)&buf, nbytes, DISKSTREAM_DEFAULT_TIMEOUT));  // push
      dt = toc(&t);
      debug("Read %s bytes: %d\r\n"
        "\t%-7.1f bytes per second (dt: %f)\r\n",
        filename, nbytes, nbytes/dt, dt );
    }
    Asynq_Token_Buffer_Free(buf);
    return 0; // success
  }

  unsigned int
  WriteMessage::config(device::HFILEDiskStreamBase *dc)
  {return 1;}

  unsigned int
  WriteMessage::run(device::HFILEDiskStreamBase *dc)
  { asynq *q  = dc->_in->contents[0];
    void *buf = Asynq_Token_Buffer_Alloc(q);
    DWORD nbytes = q->q->buffer_size_bytes;

    TicTocTimer t = tic();
    while( !dc->_agent->is_stopping() && Asynq_Pop(q, &buf, nbytes) )
      { double dt = toc(&t);
        nbytes = ((Message*)buf)->size_bytes();
        disk_stream_debug("FPS: %3.1f Frame time: %5.4f            MB/s: %3.1f Q: %3d Write %8d bytes to %s\r\n",
                1.0/dt, dt,                      nbytes/1000000.0/dt,
                q->q->head - q->q->tail,nbytes, dc->path );
        ((Message*)buf)->to_file(dc->_hfile);
      }
    Asynq_Token_Buffer_Free(buf);
    return 0; // success
  }

  unsigned int
  WriteMessageAsRaw::config(device::HFILEDiskStreamBase *dc)
  {return 1;}

  unsigned int
  WriteMessageAsRaw::run(device::HFILEDiskStreamBase *dc)
  { asynq *q  = dc->_in->contents[0];
    Message *buf = (Message*) Asynq_Token_Buffer_Alloc(q);
    DWORD nbytes = q->q->buffer_size_bytes;
    DWORD written;

    TicTocTimer t = tic();
    while(!dc->_agent->is_stopping() && Asynq_Pop(q, (void**)&buf, nbytes) )
      { double dt = toc(&t);
        nbytes = buf->size_bytes() - buf->self_size;
        disk_stream_debug("FPS: %3.1f Frame time: %5.4f            MB/s: %3.1f Q: %3d Write %8d bytes to %s\r\n",
                1.0/dt, dt,                      nbytes/1000000.0/dt,
                q->q->head - q->q->tail,nbytes, dc->path );
        Guarded_Assert_WinErr( WriteFile( dc->_hfile, buf->data, nbytes, &written, NULL ));
        Guarded_Assert( written == nbytes );
      }
    Asynq_Token_Buffer_Free(buf);
    return 0; // success
  }

  //
  // Tiff Reader/Writer
  //

  unsigned int TiffStreamReadTask::config( device::TiffStream *dc )
  {
    return 1;
  }
#if 0
  // alloc queues
  Tiff_IFD *ifd = NULL;
  Tiff_Image *tim = NULL;
  goto_if_fail(ifd=Read_Tiff_IFD(_tiff_reader),FailedIFD);
  goto_if_fail(tim=Get_Tiff_Image(ifd),FailedImageGet);    // this loads everything but the data
  Frame_With_Interleaved_Planes fmt(tim->width,tim->height,tim->number_channels,mytiff::pixel_type(tim));
  Free_Tiff_Image(tim);
  Free_Tiff_IFD(ifd);
  Rewind_Tiff_Reader(_tiff_reader);
  mytiff::unlock();
  if(_out==NULL)
    _alloc_qs_easy(&_out,1,4,fmt.size_bytes());
#endif

  unsigned int TiffStreamReadTask::run( device::TiffStream *dc )
  {
    asynq *q = dc->_out->contents[0];
    Tiff_IFD *ifd;
    Tiff_Image *tim;
    Tiff_Reader *tif = dc->_tiff_reader;
    Frame_With_Interleaved_Planes *buf = NULL;
    size_t maxsize = 0,
           maxchan = 0;
    u8 **planes = NULL;

    // compute max image size
    { 
      mytiff::lock();
      while(!End_Of_Tiff(dc->_tiff_reader))
      {
        goto_if_fail(ifd=Read_Tiff_IFD(tif),FailedIFDRead);
        goto_if_fail(tim=Get_Tiff_Image(ifd),FailedImageGet);
        Frame_With_Interleaved_Planes fmt(tim->width,tim->height,tim->number_channels,mytiff::pixel_type(tim));
        Free_Tiff_Image(tim);
        Free_Tiff_IFD(ifd);
        maxsize = max(maxsize,fmt.size_bytes());
        maxchan = max(maxchan,tim->number_channels);
      }
      Rewind_Tiff_Reader(tif);
      mytiff::unlock();
    }

    // Resize Queue if necessary
    Asynq_Resize_Buffers(q,maxsize);                                   // Make sure the queue's sized right
    buf = (Frame_With_Interleaved_Planes*)Asynq_Token_Buffer_Alloc(q); // get the first container

    // Producer loop
    planes = (u8**)malloc(sizeof(u8*)*maxchan);
    size_t nbytes = maxsize;
    size_t pitches[4];
    while (nbytes && !dc->_agent->is_stopping() && !End_Of_Tiff(tif))
    {
      mytiff::lock();
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
      mytiff::unlock();
      goto_if_fail(Asynq_Push_Timed(q, (void**)&buf, nbytes, DISKSTREAM_DEFAULT_TIMEOUT),FailedPush);  // push
    }
    Asynq_Token_Buffer_Free(buf);
    return 0; // success
FailedPush:
    Asynq_Token_Buffer_Free(buf);
    if(planes) free(planes);
    error("Failed push while reading Tiff\r\n.");
    return 1; //fail
FailedImageGet:
    warning("Failed to interpret IFD (0x%p) from TIFF while streaming.\r\n\tGot: %s\r\n\t In: %s\r\n",ifd,Tiff_Error_String(),Tiff_Error_Source());
    Free_Tiff_IFD(ifd);
    mytiff::unlock();
    Asynq_Token_Buffer_Free(buf);
    if(planes) free(planes);
    return 1; //fail
FailedIFDRead:
    warning("Failed to read IFD from TIFF while streaming.\r\n\tGot: %s\r\n\t In: %s\r\n",Tiff_Error_String(),Tiff_Error_Source());
    mytiff::unlock();
    Asynq_Token_Buffer_Free(buf);
    if(planes) free(planes);
    return 1; //fail
  }

  unsigned int TiffStreamWriteTask::config( device::TiffStream *dc )
  {
    return 1;
  }
  unsigned int TiffStreamWriteTask::run( device::TiffStream *dc )
  {
    asynq *q  = dc->_in->contents[0];
    Frame_With_Interleaved_Planes *buf = (Frame_With_Interleaved_Planes*)Asynq_Token_Buffer_Alloc(q);
    Tiff_Writer *tif = dc->_tiff_writer;
    Tiff_Image *tim;
    Tiff_IFD *ifd;
    size_t nbytes = q->q->buffer_size_bytes;

    while( !dc->_agent->is_stopping() && Asynq_Pop(q, (void**)&buf, nbytes) )
    { 
      goto_if_fail(buf->id==FRAME_INTERLEAVED_PLANES,FailureBufferHasWrongFormat);
      nbytes = buf->size_bytes();
      mytiff::lock();
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
          goto_if_fail(Add_Tiff_Image_Channel(tim,CHAN_BLACK,scale,type,data+i*pp),FailedAddChannel);
        goto_if_fail(Make_IFD_For_Image(tim,LZW_COMPRESS,buf->width,buf->height),FailedMakeIFD);
        goto_if_fail(Write_Tiff_IFD(tif,ifd)==0,FailedWriteIFD);
      }
      mytiff::unlock();
    }

    Asynq_Token_Buffer_Free(buf);
    return 0; // success
FailureBufferHasWrongFormat:
    warning("Received a buffer from the input queue with an unexpected format. (wanted FRAME_INTERLEAVED_PLANES)\r\n\tGot: %d\r\n",buf->id);
    Asynq_Token_Buffer_Free(buf);
    return 1; // fail
FailedCreateTIFFImage:
    warning("Failed to create TIFF image while streaming.\r\n\tGot: %s\r\n\t In: %s\r\n",Tiff_Error_String(),Tiff_Error_Source());
    mytiff::unlock();
    Asynq_Token_Buffer_Free(buf);
    return 1; // fail
FailedAddChannel:
    warning("Adding a channel to a tiff image (0x%p) failed.\r\n\tGot: %s\r\n\t In: %s\r\n",tim,Tiff_Error_String(),Tiff_Error_Source());
    Free_Tiff_Image(tim);
    mytiff::unlock();
    Asynq_Token_Buffer_Free(buf);
    return 1; // fail
FailedMakeIFD:
    warning("Failed to make Tiff IFD for tiff image (0x%p).\r\n\tGot: %s\r\n\t In: %s\r\n",tim,Tiff_Error_String(),Tiff_Error_Source());
    Free_Tiff_Image(tim);
    mytiff::unlock();
    Asynq_Token_Buffer_Free(buf);
    return 1; // fail
FailedWriteIFD:
    warning("Write Tiff IFD failed. Reader: 0x%p IFD: 0x%p.\r\n\tGot: %s\r\n\t In: %s\r\n",tim,ifd,Tiff_Error_String(),Tiff_Error_Source());
    Free_Tiff_IFD(ifd);
    Free_Tiff_Image(tim);
    mytiff::unlock();
    Asynq_Token_Buffer_Free(buf);
    return 1; // fail
  }

}  // namespace file
}  // namespace task
}  // namespace fetch


