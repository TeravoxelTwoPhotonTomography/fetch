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
	
	  template class DiskStream<task::file::ReadMessage,task::file::WriteMessage>;      // DiskStreamMessage;
    template class DiskStream<task::file::ReadRaw    ,task::file::WriteRaw>;          // DiskStreamRaw;    
    template class DiskStream<task::file::ReadRaw    ,task::file::WriteMessageAsRaw>; // DiskStreamMessageAsRaw;
	
    DiskStreamBase::DiskStreamBase(Agent *agent)
      :IConfigurableDevice<Config>(agent)
      ,_hfile(INVALID_HANDLE_VALUE)
    {
    }

    DiskStreamBase::DiskStreamBase( Agent *agent, Config *config )
      :IConfigurableDevice<Config>(agent)
      ,_hfile(INVALID_HANDLE_VALUE)
    {
    }

	
    DiskStreamBase::DiskStreamBase(Agent*agent, char *fname, char *m)
      :IConfigurableDevice<Config>(agent)
      ,_hfile(INVALID_HANDLE_VALUE)
    {
      _config->set_path(fname);
      _config->set_mode(m);
    }

	
    DiskStreamBase::~DiskStreamBase()
    {
    }

	
    unsigned int DiskStreamBase::attach(void)
    {
      DWORD desired_access, share_mode, creation_disposition, flags_and_attr;
      unsigned int eflag = 0; //success
      const char *mode,*filename;
      Config c = get_config();
      mode = c.mode().c_str();
      filename = c.path().c_str();

      switch (mode[0])
      {
      case 'r':
        desired_access       = GENERIC_READ;
        share_mode           = FILE_SHARE_READ; //other processes can read
        creation_disposition = OPEN_EXISTING;
        flags_and_attr = 0;
        if(_out==NULL)
          _alloc_qs_easy(&_out,1,4,1024);
        debug("Attempting to open %s for reading.\r\n",filename);
        break;
      case 'w':
        desired_access       = GENERIC_WRITE;
        share_mode           = 0;               //don't share
        creation_disposition = CREATE_ALWAYS;
        flags_and_attr       = FILE_ATTRIBUTE_NORMAL;
        if(_in==NULL)
          _alloc_qs_easy(&_in,1,4,1024);
        debug("Attempting to open %s for writing.\r\n",filename);
        break;
      default:
        { 
          warning("DiskStreamBase::attach() -- Couldn't interpret mode.  Got %s\r\n",mode);
          return 1; //failure
        }
      }

      _hfile = CreateFileA(filename,
        desired_access,
        share_mode,
        NULL,
        creation_disposition,
        flags_and_attr,
        NULL );
      if (_hfile == INVALID_HANDLE_VALUE)
      {
        ReportLastWindowsError();
        warning("Could not open file\r\n"
          "\tat %s\r\n"
          "\twith mode %s. \r\n", filename, mode);
        eflag = 1; //failure
      } else
      { 
        debug("Successfully opened file: %s\r\n",filename);
      }

      return eflag;
    }

	
    unsigned int DiskStreamBase::detach(void)
    {
      unsigned int eflag = 1; //error
      const char *mode,*filename;
      Config c = get_config();
      mode = c.mode().c_str();
      filename = c.path().c_str();
      debug("Disk Stream: %s\r\n"
            "\tAttempting to close file.\r\n", filename );
      
      // Flush
      if(_in)
      { asynq **beg =       _in->contents,
              **cur = beg + _in->nelem;
        while(cur-- > beg)
          Asynq_Flush_Waiting_Consumers( cur[0] );
      }

      // Close the file
      { 
        if( _hfile != INVALID_HANDLE_VALUE)
        { 
          if(!CloseHandle(_hfile))
          {
            ReportLastWindowsError();
            goto Error;
          }
          _hfile = INVALID_HANDLE_VALUE;
        }
      }

      eflag = 0;  // success
      debug("Disk Stream: Detached %s\r\n",filename);
Error:
      warning("Disk Stream: Trouble detaching %s\r\n",filename);
      return eflag;
    }

    
    unsigned int
    DiskStreamBase::close(void)
    { return detach();
    }

    template<typename TReader,typename TWriter>
    unsigned int 
      DiskStream<TReader,TWriter>::
      open(const std::string& filename, const std::string& mode)
    { int   sts = 1; //success

      Guarded_Assert(filename.length()<DISKSTREAM_MAX_PATH);
      Guarded_Assert(mode.length()<DISKSTREAM_MAX_MODE);
      Config c = get_config();
      c.set_path(filename);
      c.set_mode(mode);
      set_config(c);

      _agent->detach(); // allows open() to be called from any state
      if( _agent->attach()!=0 )  // open's the file handle
        return 0;// failure to open                      

      // Open the file
      // Associate read/write task
      switch (mode.c_str()[0])
      {
        case 'r':
          Guarded_Assert( _agent->arm(&reader,this,DISKSTREAM_DEFAULT_TIMEOUT) );
        break;
        case 'w':
          Guarded_Assert( _agent->arm(&writer,this,DISKSTREAM_DEFAULT_TIMEOUT) );
        break;
        default:
          error("DiskStreamBase::open() -- Couldn't interpret mode.  Got %s\r\n",mode);
      }

      _agent->run();
      return sts;
    }

  }
}
