/*
 * Microscope.cpp
 *
 * Author: Nathan Clack <clackn@janelia.hhmi.org>
 *   Date: Apr 28, 2010
 */
/*
 * Copyright 2010 Howard Hughes Medical Institute.
 * All rights reserved.
 * Use is subject to Janelia Farm Research Campus Software Copyright 1.1
 * license terms (http://license.janelia.org/license/jfrc_copyright_1_1.html).
 */
 
 #include "stdafx.h"
 #include "Microscope.h"
 #include <time.h>
 
 namespace fetch
{ namespace device {

    Microscope::Microscope()
      :IConfigurableDevice<Config>(&__self_agent)
      ,__self_agent(NULL)
      ,__scan_agent(&scanner)
      ,__io_agent(&disk)
      ,scanner(&__scan_agent)    
      ,disk(&__io_agent)
    {      
      set_config(_config);
      __common_setup();
    }  

    Microscope::Microscope( const Config &cfg )
      :IConfigurableDevice<Config>(&__self_agent)
      ,__self_agent(NULL)
      ,__scan_agent(&scanner)
      ,__io_agent(&disk)
      ,scanner(&__scan_agent)    
      ,disk(&__io_agent)
    {
      set_config(cfg);
      __common_setup();
    }

    Microscope::Microscope(Config *cfg )
      :IConfigurableDevice<Config>(&__self_agent,cfg)
      ,__self_agent(NULL)
      ,__scan_agent(&scanner)
      ,__io_agent(&disk)
      ,scanner(&__scan_agent,cfg->mutable_scanner3d())
      ,disk(&__io_agent)
      ,frame_averager(cfg->mutable_frame_average())
      ,pixel_averager(cfg->mutable_horizontal_downsample())
      ,wrap(cfg->mutable_resonant_wrap())
      ,file_series(cfg->mutable_file_series())
    {
      __common_setup();
    }


    Microscope::~Microscope(void)
    { 
    } 

    unsigned int
    Microscope::attach(void)
    { int eflag = 0; // 0 success, 1 failure
      eflag |= scanner.attach();

      std::string stackname = _config->file_prefix()+_config->stack_extension();
      file_series.ensurePathExists();
      eflag |= disk.open(file_series.getFullPath(stackname),"w");    

      return eflag;  
    }
    
    unsigned int
    Microscope::detach(void)
    { 
      int eflag = 0; // 0 success, 1 failure
      eflag |= scanner.detach(); //scanner.detach();
      eflag |= frame_averager.detach();
      eflag |= pixel_averager.detach();
      eflag |= inverter.detach();
      eflag |= trash.detach();
      eflag |= disk.detach();
      return eflag;  
    }
    
    unsigned int Microscope::disarm()
    {
      unsigned int sts = 1; // success      
      sts &= scanner.disarm();
      sts &= frame_averager.disarm();
      sts &= pixel_averager.disarm();
      sts &= inverter.disarm();
      sts &= trash.disarm();
      sts &= disk.disarm();
      
      return sts;
    }
    
    const std::string Microscope::next_filename()
    {     
      transaction_lock();
      file_series.inc();
      std::string stackname = _config->file_prefix()+_config->stack_extension();
      transaction_unlock();
      return file_series.getFullPath(stackname);
    }

    void Microscope::_set_config( Config IN *cfg )
    {
      scanner._set_config(cfg->mutable_scanner3d());
      file_series._desc = cfg->mutable_file_series();
    }

    IDevice* Microscope::configPipeline()
    {
      //Assemble pipeline here
      IDevice *cur;
      cur = &scanner;
      cur =  pixel_averager.apply(cur);
      cur =  frame_averager.apply(cur);
      cur =  inverter.apply(cur);
      cur =  cast_to_i16.apply(cur);
      cur =  wrap.apply(cur);
      return &wrap;
    }

    void Microscope::__common_setup()
    {
      __self_agent._owner = this;
      Guarded_Assert(_agent->attach()==0);
      Guarded_Assert(_agent->arm(&interaction_task,this,INFINITE));
    }

    ////////////////////////////////////
    
    FileSeries& FileSeries::inc( void )
    {
      int n = _desc->seriesno();
      _desc->set_seriesno(n+1);      
      return *this;
    }

    const std::string FileSeries::getFullPath( const std::string& shortname )
    {
      char strSeriesNo[32];
      renderSeriesNo(strSeriesNo,sizeof(strSeriesNo));
      updateDate();

      // reset series number when series path changes
      std::string seriespath = _desc->root() + _desc->pathsep() + _desc->date();
      if(seriespath.compare(_lastpath)!=0)
        _desc->set_seriesno(0); // for next time
      _lastpath = seriespath;

      return seriespath
        + _desc->pathsep()
        + strSeriesNo
        + _desc->pathsep()
        + shortname; 
    }

    void FileSeries::updateDate( void )
    {
      time_t clock = time(NULL);
      struct tm *t = localtime(&clock);
      char datestr[] = "0000-00-00";      
      sprintf_s(datestr,sizeof(datestr),"%04d-%02d-%02d",t->tm_year+1900,t->tm_mon+1,t->tm_mday);
      _desc->set_date(datestr);
    }
   
    void FileSeries::ensurePathExists()
    {
      std::string t;
      char strSeriesNo[32];

      renderSeriesNo(strSeriesNo,sizeof(strSeriesNo));
      updateDate();
      
      t = _desc->root()+_desc->pathsep()+_desc->date();
      tryCreateDirectory(t.c_str(), "root path", _desc->root().c_str());

      t += _desc->pathsep()+strSeriesNo;
      tryCreateDirectory(t.c_str(), "date path", _desc->root().c_str());
    }

    void FileSeries::renderSeriesNo( char * strSeriesNo,int maxbytes )
    {
      int n = _desc->seriesno();
      if(n>99999)
        warning("File series number is greater than the supported number of digits.\r\n");
      memset(strSeriesNo,0,maxbytes);
      sprintf_s(strSeriesNo,maxbytes,"%05d",n); //limited to 5 digits
    }

    void FileSeries::tryCreateDirectory( LPCTSTR path, const char* description, LPCTSTR root )
    {
      if(!CreateDirectory(path,NULL/*default security*/))
      {
        switch(GetLastError())
        {
        case ERROR_ALREADY_EXISTS: /*ignore*/ 
          break;
        case ERROR_PATH_NOT_FOUND:
          error("FileSeries: Could not find %s:\r\n\t%s\r\n",description,root);
          break;
        default:
          error("Unexpected error returned after call to CreateDirectory()\r\n");
        }
      }
    }

    //notes
    //-----
    // - I can see no reason right now to use transactions
    //   It would prevent against renaming/deleting dependent directories during the transaction
    //   Might be interesting in other places to lock files during acquisition.  However, I think a little
    //   discipline from the user(me) can go a long way.
    //
    // - approach
    //
    //   1. assert <root> exists
    //   2. assert creation or existence of <root>/<date>
    //   3. assert creation of <root>/<data>/<seriesno>
    //   4. return true on success, fail otherwise.
    //see     
    //---
    //
    //MSDN CreateDirectory http://msdn.microsoft.com/en-us/library/aa363855(VS.85).aspx
    //MSDN CreateDirectoryTransacted http://msdn.microsoft.com/en-us/library/aa363855(VS.85).aspx
    //MSDN CreateTransaction http://msdn.microsoft.com/en-us/library/aa366011(v=VS.85).aspx
    //     handle = CreateTransaction(NULL/*default security*/,0,0,0,0,0/*timeout,0==inf*/,"description");


  } // end namespace device  
} // end namespace fetch
