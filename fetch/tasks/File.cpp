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
#include "File.h"

#if 0
#define disk_stream_debug(...) debug(__VA_ARGS__)
#else
#define disk_stream_debug(...)
#endif

namespace fetch {
namespace task {
namespace file {


  unsigned int
  ReadRaw::config(device::DiskStream *agent)
  {return 1;}



  unsigned int
  ReadRaw::run(device::DiskStream *agent)
  { asynq *q  = agent->out->contents[0];
    void *buf = Asynq_Token_Buffer_Alloc(q);
    DWORD nbytes = q->q->buffer_size_bytes,
          bytes_read;
    Disk_Stream *stream = (Disk_Stream*) d->context;
    TicTocTimer t = tic();
    do
    { double dt;
      Guarded_Assert_WinErr(
        ReadFile( stream->hfile, buf, nbytes, &bytes_read, NULL ));
      Guarded_Assert( Asynq_Push_Timed(q, &buf, nbytes, DISK_STREAM_DEFAULT_TIMEOUT) );
      dt = toc(&t);
      debug("Read %s bytes: %d\r\n"
            "\t%-7.1f bytes per second (dt: %f)\r\n",
            stream->path, nbytes, nbytes/dt, dt );
    } while ( nbytes && WAIT_OBJECT_0 != WaitForSingleObject(d->notify_stop, 0) );
    Asynq_Token_Buffer_Free(buf);
    return 0; // success
  }



  unsigned int
  WriteRaw::config(device::DiskStream *agent)
  {return 1;}



  unsigned int
  WriteRaw::run(device::DiskStream *agent)
  { asynq *q  = agent->in->contents[0];
    void *buf = Asynq_Token_Buffer_Alloc(q);
    DWORD nbytes = q->q->buffer_size_bytes,
          written;

    TicTocTimer t = tic();
    do
    { while( Asynq_Pop_Try(q, &buf) )
      { double dt = toc(&t);
        nbytes = q->q->buffer_size_bytes;
        disk_stream_debug("FPS: %3.1f Frame time: %5.4f            MB/s: %3.1f Q: %3d Write %8d bytes to %s\r\n",
                1.0/dt, dt,                      nbytes/1000000.0/dt,
                q->q->head - q->q->tail,nbytes, stream->path );
        Guarded_Assert_WinErr( WriteFile( stream->hfile, buf, nbytes, &written, NULL ));
        Guarded_Assert( written == nbytes );
      }
    } while ( WAIT_OBJECT_0 != WaitForSingleObject(d->notify_stop, 0) );
    Asynq_Token_Buffer_Free(buf);
    return 0; // success
  }

/*
 * [ ] rewrite read to use message to/from file
 * [ ] rewrite write to use message to/from file
 *
 */
  unsigned int
  ReadMessage::config(device::DiskStream *agent)
  {return 1;}

  unsigned int
  ReadMessage::run(device::DiskStream *agent)
  { asynq *q  = agent->out->contents[0];
    Message *buf = (Message*)Asynq_Token_Buffer_Alloc(q);
    DWORD nbytes,
          bytes_read;
    TicTocTimer t = tic();
    do
    { double dt;
      Guarded_Assert( nbytes = Message::from_file(agent->hfile,NULL,0));                   // get size
      Guarded_Assert(      0== Message::from_file(agent->hfile,buf,nbytes));               // read data
      Guarded_Assert( Asynq_Push_Timed(q, &buf, nbytes, DISK_STREAM_DEFAULT_TIMEOUT) );    // push
      dt = toc(&t);
      debug("Read %s bytes: %d\r\n"
            "\t%-7.1f bytes per second (dt: %f)\r\n",
            stream->path, nbytes, nbytes/dt, dt );
    } while ( nbytes && WAIT_OBJECT_0 != WaitForSingleObject(d->notify_stop, 0) );
    Asynq_Token_Buffer_Free(buf);
    return 0; // success
  }



  unsigned int
  WriteMessage::config(device::DiskStream *agent)
  {return 1;}



  unsigned int
  WriteMessage::run(device::DiskStream *agent)
  { asynq *q  = agent->in->contents[0];
    void *buf = Asynq_Token_Buffer_Alloc(q);
    DWORD nbytes = q->q->buffer_size_bytes,
          written;

    TicTocTimer t = tic();
    do
    { while( Asynq_Pop_Try(q, &buf, nbytes) )
      { double dt = toc(&t);
        nbytes = ((Message*)buf)->size_bytes();
        disk_stream_debug("FPS: %3.1f Frame time: %5.4f            MB/s: %3.1f Q: %3d Write %8d bytes to %s\r\n",
                1.0/dt, dt,                      nbytes/1000000.0/dt,
                q->q->head - q->q->tail,nbytes, stream->path );
        ((Message*)buf)->to_file(agent->hfile);
      }
    } while ( WAIT_OBJECT_0 != WaitForSingleObject(d->notify_stop, 0) );
    Asynq_Token_Buffer_Free(buf);
    return 0; // success
  }
}  // namespace file
}  // namespace task
}  // namespace fetch


