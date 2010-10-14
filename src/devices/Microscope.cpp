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
      Guarded_Assert(_agent->attach());
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

      int n = _desc->seriesno();
      if(n>99999)
        warning("File series number is greater than the supported number of digits.\r\n");
      memset(strSeriesNo,0,sizeof(strSeriesNo));
      sprintf_s(strSeriesNo,sizeof(strSeriesNo),"%05d",n); //limited to 5 digits

      updateDate();

      // reset series number when series path changes
      std::string seriespath = _desc->root() + _desc->pathsep() + _desc->date();
      if(seriespath.compare(_lastpath)!=0)
        _desc->set_seriesno(0);
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

  } // end namespace device  
} // end namespace fetch
