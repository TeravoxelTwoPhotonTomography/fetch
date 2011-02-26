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

#include "devices/DiskStream.h"
#include "frame.h"
#include "File.h"
#include "util/util-file.h"
#if 0
#define disk_stream_debug(...) debug(__VA_ARGS__)
#else
#define disk_stream_debug(...)
#endif
namespace mylib {
#include "MY_TIFF/tiff.image.h"
}
#include "util/util-mylib.h"

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
        nbytes = Chan_Buffer_Size_Bytes(q);
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
        maxsize = max( maxsize, sz );                            // update the max size
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

  unsigned int
  WriteMessage::config(device::HFILEDiskStreamBase *dc)
  {return 1;}

  unsigned int
  WriteMessage::run(device::HFILEDiskStreamBase *dc)
  { Chan *q   = Chan_Open(dc->_in->contents[0],CHAN_READ);
    void *buf = Chan_Token_Buffer_Alloc(q);
    DWORD nbytes = Chan_Buffer_Size_Bytes(q);

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

  //
  // Tiff Reader/Writer
  //

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
    size_t maxsize = 0,
           maxchan = 0;
    u8 **planes = NULL;
    int eflag=0;

    // compute max image size
    { 
      
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

  //#undef  goto_if_fail
  //#define goto_if_fail( cond, lbl )       { if(!(cond)) {breakme();goto lbl;} }
  //void breakme() 
  //{ HERE;
  //}
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

    while(CHAN_SUCCESS( Chan_Next(q,(void**)&buf,nbytes) ))      //!dc->_agent->is_stopping() && 
    { 
      goto_if_fail(buf->id==FRAME_INTERLEAVED_PLANES,FailureBufferHasWrongFormat);
      nbytes = buf->size_bytes();

      //buf->dump("TiffStreamWriteTask-src.%s",TypeStrFromID(buf->rtti));          
      
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
        goto_if_fail(ifd=Make_IFD_For_Image(tim,DONT_COMPRESS,buf->width,buf->height),FailedMakeIFD);
        goto_if_fail(Write_Tiff_IFD(tif,ifd)==0,FailedWriteIFD);
      }
      
    }
    eflag=0; //success
Finalize:
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

}  // namespace file
}  // namespace task
}  // namespace fetch


