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
#include "util/util-mylib.h"
#include "DiskStream.h"
#include "tasks/File.h"
#include "Chan.h"
#include <Windows.h>

#include "frame.h"
#include "types.h"

namespace fetch
{

  bool operator==(const cfg::File& a, const cfg::File& b)         {return equals(&a,&b);}
  bool operator!=(const cfg::File& a, const cfg::File& b)         {return !(a==b);}

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
      warning("IDiskStream::flus h() called.  Need to be sure this is necessary.  Could lose data.\r\n");
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
    BOOL test;

    // 64 bits integer, low and high bytes
    __int64 lpFreeBytesAvailable, lpTotalNumberOfBytes, lpTotalNumberOfFreeBytes; 

    std::string stemp = std::string(filename.begin(), filename.begin()+3);
    LPCTSTR pszDrive = stemp.c_str();

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
        Guarded_Assert( _agent->arm(_reader,this,DISKSTREAM_DEFAULT_TIMEOUT)==0 );
        break;
      case 'w':
        // If the function succeeds, the return value is nonzero. If the function fails, the return value is 0 (zero).
        test = GetDiskFreeSpaceEx(
          pszDrive,
          (PULARGE_INTEGER)&lpFreeBytesAvailable,
          (PULARGE_INTEGER)&lpTotalNumberOfBytes,
          (PULARGE_INTEGER)&lpTotalNumberOfFreeBytes
          );

        // JL03212014 Check whether the free diskspace is larger than 10G, if not, report error
        if (test==0 || lpTotalNumberOfFreeBytes < DISKSTREAM_CURRENT_FREEBYTES) {
          warning("IDiskStream::open() -- The free diskspace is less than 10G, stop writing.");
          eflag = 1;
        }
        else 
          Guarded_Assert( _agent->arm(_writer,this,DISKSTREAM_DEFAULT_TIMEOUT)==0 );

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
        mylib::Close_Tiff(_tiff_writer);
        _tiff_writer = NULL;
      }
      if(_tiff_reader)
      {
        mylib::Free_Tiff_Reader(_tiff_reader);
        _tiff_reader = NULL;
      }
      return 0; //success
    }

#pragma warning(push)
#pragma warning(disable:4533) // initialization is skipped by GOTO
    unsigned int TiffStream::_attach_reader( char * filename )
    {
      Guarded_Assert(_tiff_reader); // open() should call on_detach() before on_attach().  on_detach() should set open handles to null.
      
      goto_if_fail(_tiff_reader = mylib::Open_Tiff_Reader(filename,NULL,NULL,mytiff::islsm(filename)),FailedOpen);   

      // alloc queues
      mylib::Tiff_IFD *ifd = NULL;
      mylib::Tiff_Image *tim = NULL;
      goto_if_fail(ifd=mylib::Read_Tiff_IFD(_tiff_reader),FailedIFD);
      goto_if_fail(tim=mylib::Get_Tiff_Image(ifd),FailedImageGet);    // this loads everything but the data
      Frame_With_Interleaved_Planes fmt(tim->width,tim->height,tim->number_channels,mytiff::pixel_type(tim));
      mylib::Free_Tiff_Image(tim);
      mylib::Free_Tiff_IFD(ifd);
      mylib::Rewind_Tiff_Reader(_tiff_reader);
      
      if(_out==NULL)
        _alloc_qs_easy(&_out,1,4,fmt.size_bytes());
      return 0; //success
FailedOpen:
      warning("Could not open %s for TIFF reading.\r\n",filename);
      
      return 1; //failure
FailedIFD:
      warning("Failed to read first IFD in %s\r\n",filename);
      mylib::Free_Tiff_Reader(_tiff_reader);
      _tiff_reader = NULL;
      
      return 1; //failure
FailedImageGet:
      warning("Failed to read image params from first IFD (0x%p).\r\n\tFile: %s\r\n",ifd,filename);
      mylib::Free_Tiff_IFD(ifd);
      mylib::Free_Tiff_Reader(_tiff_reader);
      _tiff_reader = NULL;
      
      return 1; //failure
    }
#pragma warning(pop)

#pragma warning(push)
#pragma warning(disable:4533) // initialization is skipped by GOTO
    unsigned int TiffStream::_attach_writer( char * filename )
    {
      Guarded_Assert(_tiff_writer==NULL); // open() should call on_detach() before on_attach().  on_detach() should set open handles to null.
      
      goto_if_fail(
        _tiff_writer = mylib::Open_Tiff_Writer(filename,FALSE/*32 bit*/,mytiff::islsm(filename)),
        FailedOpen);
      

      Frame_With_Interleaved_Planes fmt(1024,1024,3,id_u16);
      if(_in==NULL)
        _alloc_qs_easy(&_in,1,4,fmt.size_bytes());

      return 0; //success
FailedOpen:
      warning("Could not open %s for TIFF writing.\r\n",filename);
      
      return 1; //failure
    }
#pragma warning(pop)

///////////////////////////////////////////////////////////////////////////////
//  TiffGroupStream  //////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
#define ENDL "\r\n"
#define TRY(e) if(!(e)) {warning("%s(%d):"ENDL "\t%s"ENDL "\tExpression evaluated to false."ENDL,__FILE__,__LINE__,#e); goto Error;}

    // Constructor  ///////////////////////////////////////////////////////////
    TiffGroupStream::TiffGroupStream( Agent *agent )
      :IDiskStream(agent),
       nchan_(0)
    {
      _writer = &_write_task;
      _reader = &_read_task;
    }

    // Constructor  ///////////////////////////////////////////////////////////
    /** \param[in] config specifies the path and mode of a file */
    TiffGroupStream::TiffGroupStream( Agent *agent, Config *config )
      :IDiskStream(agent,config),
       nchan_(0)
    {
      _writer = &_write_task;
      _reader = &_read_task;
    }

    // on_attach  /////////////////////////////////////////////////////////////
    /** called (indirectly) by IDiskStream::open() */
    unsigned int TiffGroupStream::on_attach()
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

    // on_detach  /////////////////////////////////////////////////////////////
    /** called (indirectly) by IDiskStream::open() */
    unsigned int TiffGroupStream::on_detach()
    { int i;

      for(i=0;i<_readers.size();++i)
         if(_readers[i]) mylib::Close_Tiff(_readers[i]);
      _readers.clear();

      for(i=0;i<_writers.size();++i)
         if(_writers[i]) mylib::Close_Tiff(_writers[i]);
      _writers.clear();
      return 0; //success
    }

    // _attach_reader /////////////////////////////////////////////////////////
    unsigned int TiffGroupStream::_attach_reader(char *filename)
    { int eflag=0;
      TRY(_readers.empty()==true);  // open() should call on_detach() before on_attach().  on_detach() should close all open handles and clear this vector
      error("%s(%d): "ENDL "\tNot implemented."ENDL,__FILE__,__LINE__);
Finalize:
      return eflag;
Error:
      eflag=1;
      goto Finalize;
    }

    // _attach_writer /////////////////////////////////////////////////////////
    unsigned int TiffGroupStream::_attach_writer(char *filename)
    { int eflag=0;     
      Frame_With_Interleaved_Planes fmt(1024,1024,3,id_u16);
      TRY(_writers.empty()==true);  // open() should call on_detach() before on_attach().  on_detach() should close all open handles and clear this vector

      warning("%s(%d): "ENDL "\tNot tested.  Should test for destination writeable."ENDL,__FILE__,__LINE__);

      if(_in==NULL)
        _alloc_qs_easy(&_in,1,4,fmt.size_bytes());
    
Finalize:
      return eflag;
Error:
      eflag=1;
      goto Finalize;
    }
    
  }
}
