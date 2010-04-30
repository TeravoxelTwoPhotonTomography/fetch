/*
 * DiskStream.cpp
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
/*
 * open
 *    - Can be called from any state.
 *    - Will close an open file (stop,disarm and detach) and replace it with
 *      the new file.
 *    - will arm, and run
 */
#include "stdafx.h"
#include "DiskStream.h"
#include "../tasks/File.h"

namespace fetch
{ 
  namespace device
  {
	
    DiskStream::DiskStream()
      : hfile(INVALID_HANDLE_VALUE)
    { //reader = new TReader();
      //writer = new TWriter();
    }

	
    DiskStream::DiskStream(char *fname, char *m)
      : hfile(INVALID_HANDLE_VALUE)
    {
      //lock();
      //reader = new task::file::ReadMessage;
      //writer = new task::file::WriteMessage;
      strncpy(this->filename,fname,DISKSTREAM_MAX_PATH);
      strncpy(this->mode,mode,DISKSTREAM_MAX_MODE);
      //unlock();
    }
    
	
    DiskStream::~DiskStream()
    { //delete reader;
      //delete writer;
    }

	
    unsigned int DiskStream::attach(void)
    { DWORD desired_access, share_mode, creation_disposition, flags_and_attr;
      unsigned int sts = 1; //success
      switch (mode[0])
      {
        case 'r':
          desired_access       = GENERIC_READ;
          share_mode           = FILE_SHARE_READ; //other processes can read
          creation_disposition = OPEN_EXISTING;
          flags_and_attr = 0;
        break;
        case 'w':
          desired_access       = GENERIC_WRITE;
          share_mode           = 0; //don't share
          creation_disposition = CREATE_ALWAYS;
          flags_and_attr       = FILE_ATTRIBUTE_NORMAL;
        break;
        default:
          error("DiskStream::attach() -- Couldn't interpret mode.  Got %s\r\n",
                mode);
      }
      
      this->lock();
      hfile = CreateFile(filename,
                         desired_access,
                         share_mode,
                         NULL,
                         creation_disposition,
                         flags_and_attr,
                         NULL );
      if (hfile == INVALID_HANDLE_VALUE)
      {
        ReportLastWindowsError();
        warning("Could not open file\r\n"
               "\tat %s\r\n"
               "\twith mode %c\r\n", filename, mode);
        sts = 0; //failure
      } else
      { this->set_available();
      }
      this->unlock();
      
      return sts;
    }

	
    unsigned int DiskStream::detach(void)
    { unsigned int sts = 0; //error

      debug("Disk Stream: %s\r\n"
            "\tAttempting to close file.\r\n", this->filename );

      // Flush
      this->lock();
      { asynq **beg =       in->contents,
              **cur = beg + in->nelem;
        while(cur-- > beg)
          Asynq_Flush_Waiting_Consumers( cur[0] );
      }
      this->unlock();

      if( !this->disarm( DISKSTREAM_DEFAULT_TIMEOUT ) )
        warning("Could not cleanly disarm disk stream.\r\n");

      // Close the file
      this->lock();
      { if( hfile != INVALID_HANDLE_VALUE)
        { if(!CloseHandle(hfile))
          { ReportLastWindowsError();
            goto Error;
          }
          hfile = INVALID_HANDLE_VALUE;
        }
      }

      sts = 1;  // success
      debug("Disk Stream: Detached %s\r\n",filename);
    Error:
      this->unlock();
      return sts;
    }

    inline unsigned int
    DiskStream::close(void)
    { return detach();
    }

    template<typename TReader,typename TWriter>
    unsigned int DiskStreamSpecialized<TReader,TWriter>::open(char *filename, char *mode)
    { int   sts = 1; //success

      Guarded_Assert(strlen(filename)<DISKSTREAM_MAX_PATH);
      Guarded_Assert(strlen(mode)<DISKSTREAM_MAX_MODE);
      this->lock();
      strncpy(this->filename,filename,DISKSTREAM_MAX_PATH);
      strncpy(this->mode,mode,DISKSTREAM_MAX_MODE);
      this->unlock();

      this->detach(); // allows open() to be called from any state
      this->attach(); // open's the file handle

      // Open the file
      // Associate read/write task
      switch (mode[0])
      {
        case 'r':
          Guarded_Assert( arm(&reader,DISKSTREAM_DEFAULT_TIMEOUT) );
        break;
        case 'w':
          Guarded_Assert( arm(&writer,DISKSTREAM_DEFAULT_TIMEOUT) );
        break;
        default:
          error("DiskStream::open() -- Couldn't interpret mode.  Got %s\r\n",
                mode);
      }

      this->run();
      return sts;
    }
    template<>
        unsigned int
        DiskStreamSpecialized<task::file::ReadMessage,task::file::WriteMessage>::
        open(char *filename, char *mode);
    template<>
		    unsigned int
		    DiskStreamSpecialized<task::file::ReadRaw,task::file::WriteRaw>::
		    open(char *filename, char *mode);
  }
}
