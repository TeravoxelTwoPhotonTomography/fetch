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
#include "../devices/DiskStream.h"
#include "../frame.h"
#include "File.h"
#include "../util/util-file.h"
#if 0
#define disk_stream_debug(...) debug(__VA_ARGS__)
#else
#define disk_stream_debug(...)
#endif

namespace fetch {
namespace task {
namespace file {


  // Upcasting
  unsigned int ReadRaw::config      (IDevice *d) {return config(dynamic_cast<device::DiskStreamBase*>(d));}
  unsigned int ReadRaw::run         (IDevice *d) {return run   (dynamic_cast<device::DiskStreamBase*>(d));}
  
  unsigned int WriteRaw::config     (IDevice *d) {return config(dynamic_cast<device::DiskStreamBase*>(d));}
  unsigned int WriteRaw::run        (IDevice *d) {return run   (dynamic_cast<device::DiskStreamBase*>(d));}
  
  unsigned int ReadMessage::config  (IDevice *d) {return config(dynamic_cast<device::DiskStreamBase*>(d));}
  unsigned int ReadMessage::run     (IDevice *d) {return run   (dynamic_cast<device::DiskStreamBase*>(d));}
  
  unsigned int WriteMessage::config (IDevice *d) {return config(dynamic_cast<device::DiskStreamBase*>(d));}
  unsigned int WriteMessage::run    (IDevice *d) {return run   (dynamic_cast<device::DiskStreamBase*>(d));}
  
  unsigned int WriteMessageAsRaw::config (IDevice *d) {return config(dynamic_cast<device::DiskStreamBase*>(d));}
  unsigned int WriteMessageAsRaw::run    (IDevice *d) {return run   (dynamic_cast<device::DiskStreamBase*>(d));}  

  //
  // Implementation
  //
  
  unsigned int
  ReadRaw::config(device::DiskStreamBase *dc)
  {return 1;}

  unsigned int
  ReadRaw::run(device::DiskStreamBase *dc)
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
  WriteRaw::config(device::DiskStreamBase *dc)
  {return 1;}



  unsigned int
  WriteRaw::run(device::DiskStreamBase *dc)
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
  ReadMessage::config(device::DiskStreamBase *dc)
  {return 1;}

  unsigned int
  ReadMessage::run(device::DiskStreamBase *dc)
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
  WriteMessage::config(device::DiskStreamBase *dc)
  {return 1;}



  unsigned int
  WriteMessage::run(device::DiskStreamBase *dc)
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
  WriteMessageAsRaw::config(device::DiskStreamBase *dc)
  {return 1;}



  unsigned int
  WriteMessageAsRaw::run(device::DiskStreamBase *dc)
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
}  // namespace file
}  // namespace task
}  // namespace fetch


