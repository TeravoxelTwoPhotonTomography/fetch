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
#include "common.h"
#include "DiskStream.h"
#include "tasks/File.h"
#include "Chan.h"
#include "MY_TIFF/tiff.io.h"
#include "util/util-mylib.h"
#include "MY_TIFF/tiff.image.h"
#include "frame.h"
#include "types.h"


namespace fetch
{ 
  namespace device
  {
	
	  template class HFILEDiskStream<task::file::ReadMessage,task::file::WriteMessage>;      // DiskStreamMessage;
    template class HFILEDiskStream<task::file::ReadRaw    ,task::file::WriteRaw>;          // DiskStreamRaw;    
    template class HFILEDiskStream<task::file::ReadRaw    ,task::file::WriteMessageAsRaw>; // DiskStreamMessageAsRaw;
	
    IDiskStream::IDiskStream(Agent *agent)
      :IConfigurableDevice<Config>(agent)
      ,_reader(NULL)
      ,_writer(NULL)
    {
    }

    IDiskStream::IDiskStream( Agent *agent, Config *config )
      :IConfigurableDevice<Config>(agent,config)
      ,_reader(NULL)
      ,_writer(NULL)
    {
    }

	
    IDiskStream::IDiskStream(Agent*agent, char *fname, char *m)
      :IConfigurableDevice<Config>(agent)
    {
      _config->set_path(fname);
      _config->set_mode(m);
    }

    void IDiskStream::flush()
    {
      warning("IDiskStream::flsuh() called.  Need to be sure this is necessary.  Could lose data.\r\n");
      if(_in)
      { 
        Chan **beg =       _in->contents,
             **cur = beg + _in->nelem;
        while(cur-- > beg)
          Guarded_Assert(Chan_Is_Empty(*cur)); //should be empty here.  If not, need to wait till emtpy          
      }
    }

    unsigned int IDiskStream::open(const std::string& filename, const std::string& mode)
    {
      int   eflag = 0; //success

      Guarded_Assert(filename.length()<DISKSTREAM_MAX_PATH);
      Guarded_Assert(mode.length()<DISKSTREAM_MAX_MODE);

      // Close the acting stream if it's open.
      _agent->detach(); // allows open() to be called from any state.

      Config c = get_config();
      c.set_path(filename);
      c.set_mode(mode);
      set_config(c);
      
      if( _agent->attach()!=0 )  // open the file handle
        return 1;// failure to open                      

      // Open the file
      // Associate read/write task
      switch (mode.c_str()[0])
      {
      case 'r':
        Guarded_Assert( _agent->arm(_reader,this,DISKSTREAM_DEFAULT_TIMEOUT) );
        break;
      case 'w':
        Guarded_Assert( _agent->arm(_writer,this,DISKSTREAM_DEFAULT_TIMEOUT) );
        break;
      default:
        error("IDiskStream::open() -- Couldn't interpret mode.  Got %s\r\n",mode);
      }

      eflag |= _agent->run()==0;
      return eflag;
    }

    HFILEDiskStreamBase::HFILEDiskStreamBase( Agent *agent )
      :IDiskStream(agent)
      ,_hfile(INVALID_HANDLE_VALUE)
    {

    }

    HFILEDiskStreamBase::HFILEDiskStreamBase( Agent *agent, Config *config )
      :IDiskStream(agent,config)
      ,_hfile(INVALID_HANDLE_VALUE)
    {

    }

    template<typename TReader,typename TWriter>
    fetch::device::HFILEDiskStream<TReader, TWriter>::HFILEDiskStream( Agent *agent, Config *config )
      :HFILEDiskStreamBase(agent,config)      
    {
      _reader = &_inst_reader;
      _writer = &_inst_writer;
    }

    template<typename TReader,typename TWriter>
    fetch::device::HFILEDiskStream<TReader, TWriter>::HFILEDiskStream( Agent *agent )
      :HFILEDiskStreamBase(agent)     
    {
      _reader = &_inst_reader;
      _writer = &_inst_writer;
    }

    template<typename TReader,typename TWriter>
    unsigned int fetch::device::HFILEDiskStream<TReader, TWriter>::on_attach( void )
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
          warning("IDiskStream::on_attach() -- Couldn't interpret mode.  Got %s\r\n",mode);
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

    template<typename TReader,typename TWriter>
    unsigned int fetch::device::HFILEDiskStream<TReader, TWriter>::on_detach( void )
    {
      unsigned int eflag = 1; //error
      const char *mode,*filename;
      Config c = get_config();
      mode = c.mode().c_str();
      filename = c.path().c_str();
      debug("Disk Stream: %s\r\n"
        "\tAttempting to close file.\r\n", filename );

      flush();

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
Finalize:      
      return eflag;
Error:
      warning("Disk Stream: Trouble detaching %s\r\n",filename);
      goto Finalize;
    }


    TiffStream::TiffStream( Agent *agent )
      :IDiskStream(agent)
      ,_tiff_reader(NULL)
      ,_tiff_writer(NULL)
    {
      _writer = &_write_task;
      _reader = &_read_task;
    }

    TiffStream::TiffStream( Agent *agent, Config *config )
      :IDiskStream(agent,config)
      ,_tiff_reader(NULL)
      ,_tiff_writer(NULL)
    {
      _writer = &_write_task;
      _reader = &_read_task;
    }

    unsigned int TiffStream::on_attach()
    {
      unsigned int eflag = 0; //success
      char *mode,*filename;
      Config c = get_config();
      mode = const_cast<char*>(c.mode().c_str());
      filename = const_cast<char*>(c.path().c_str());

      switch(mode[0])
      {
      case 'r':
        eflag |= _attach_reader(filename);
        break;
      case 'w':
        eflag |= _attach_writer(filename);
        break;
      default:
        error("Could not recognize mode.  Expected r or w.\r\n");
      }

      return eflag;
    }

    unsigned int TiffStream::on_detach()
    {
      if(_tiff_writer)
      {
        Free_Tiff_Writer(_tiff_writer);
        _tiff_writer = NULL;
      }
      if(_tiff_reader)
      {
        Free_Tiff_Reader(_tiff_reader);
        _tiff_reader = NULL;
      }
      return 0; //success
    }

#pragma warning(push)
#pragma warning(disable:4533) // initialization is skipped by GOTO
    unsigned int TiffStream::_attach_reader( char * filename )
    {
      Guarded_Assert(_tiff_reader); // open() should call on_detach() before on_attach().  on_detach() should set open handles to null.
      mytiff::lock();
      goto_if_fail(_tiff_reader = Open_Tiff_Reader(filename,NULL,NULL,mytiff::islsm(filename)),FailedOpen);   

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
      return 0; //success
FailedOpen:
      warning("Could not open %s for TIFF reading.\r\n",filename);
      mytiff::unlock();
      return 1; //failure
FailedIFD:
      warning("Failed to read first IFD in %s\r\n",filename);
      Free_Tiff_Reader(_tiff_reader);
      _tiff_reader = NULL;
      mytiff::unlock();
      return 1; //failure
FailedImageGet:
      warning("Failed to read image params from first IFD (0x%p).\r\n\tFile: %s\r\n",ifd,filename);
      Free_Tiff_IFD(ifd);
      Free_Tiff_Reader(_tiff_reader);
      _tiff_reader = NULL;
      mytiff::unlock();
      return 1; //failure
    }
#pragma warning(pop)

#pragma warning(push)
#pragma warning(disable:4533) // initialization is skipped by GOTO
    unsigned int TiffStream::_attach_writer( char * filename )
    {
      Guarded_Assert(_tiff_writer==NULL); // open() should call on_detach() before on_attach().  on_detach() should set open handles to null.
      mytiff::lock();
      goto_if_fail(_tiff_writer = Open_Tiff_Writer(filename,FALSE/*32 bit*/,mytiff::islsm(filename)),FailedOpen);
      mytiff::unlock();

      Frame_With_Interleaved_Planes fmt(1024,1024,3,id_u16);
      if(_in==NULL)
        _alloc_qs_easy(&_in,1,4,fmt.size_bytes());

      return 0; //success
FailedOpen:
      warning("Could not open %s for TIFF writing.\r\n",filename);
      mytiff::unlock();
      return 1; //failure
    }
#pragma warning(pop)

  }
}
