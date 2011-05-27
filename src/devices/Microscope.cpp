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
 
 
#include "Microscope.h"
#include <time.h>

#include <iostream>
#include <fstream>
#include "stack.pb.h"
#include "microscope.pb.h"
#include "google\protobuf\text_format.h"
 
 namespace fetch
{ namespace device {

    Microscope::Microscope()
      :IConfigurableDevice<Config>(&__self_agent)
      ,__self_agent("Microscope",NULL)
      ,__scan_agent("Scanner",&scanner)
      ,__io_agent("IO",&disk)
      ,scanner(&__scan_agent)    
      ,stage_(&__self_agent)  
      ,fov_(_config->fov())
      ,disk(&__io_agent)
      ,frame_averager("FrameAverager")
      ,pixel_averager("PixelAverager")
      ,cast_to_i16("i16Cast")
      ,inverter("inverter")
      ,wrap()
      ,trash("Trash")
    {      
      set_config(_config);
      __common_setup();
    }  
    /*
    worker::FrameAverageAgent 	   frame_averager;
    worker::HorizontalDownsampleAgent pixel_averager;
    worker::FrameCastAgent_i16     cast_to_i16;
    worker::FrameInvertAgent       inverter;
    worker::ResonantWrapAgent      wrap;

    worker::TerminalAgent		       trash;    
    */
    Microscope::Microscope( const Config &cfg )
      :IConfigurableDevice<Config>(&__self_agent)
      ,__self_agent("Microscope",NULL)
      ,__scan_agent("Scanner",&scanner)
      ,__io_agent("IO",&disk)
      ,scanner(&__scan_agent)       
      ,stage_(&__self_agent)
      ,fov_(cfg.fov())
      ,disk(&__io_agent)
      ,frame_averager("FrameAverager")
      ,pixel_averager("PixelAverager")
      ,cast_to_i16("i16Cast")
      ,inverter("inverter")
      ,wrap()
      ,trash("Trash")
      ,file_series()
    {
      set_config(cfg);
      __common_setup();
    }

    Microscope::Microscope(Config *cfg )
      :IConfigurableDevice<Config>(&__self_agent,cfg)
      ,__self_agent("Microscope",NULL)
      ,__scan_agent("Scanner",&scanner)
      ,__io_agent("IO",&disk)
      ,scanner(&__scan_agent,cfg->mutable_scanner3d())       
      ,stage_(&__self_agent,cfg->mutable_stage())
      ,fov_(cfg->fov())
      ,disk(&__io_agent)
      ,frame_averager(cfg->mutable_frame_average(),"FrameAverager")
      ,pixel_averager(cfg->mutable_horizontal_downsample(),"PixelAverager")
      ,cast_to_i16("i16Cast") 
      ,inverter("Inverter")
      ,wrap(cfg->mutable_resonant_wrap())
      ,trash("Trash")
      ,file_series(cfg->mutable_file_series())
    {
      __common_setup();
    }


    Microscope::~Microscope(void)
    { 
      if(__scan_agent.detach()) warning("Microscope __scan_agent did not detach cleanly\r\n");
      if(__self_agent.detach()) warning("Microscope __self_agent did not detach cleanly\r\n");
      if(  __io_agent.detach()) warning("Microscope __io_agent did not detach cleanly\r\n");
    } 

    unsigned int
    Microscope::on_attach(void)
    { 
      int eflag = 0; // 0 success, 1 failure

      // argh this is a confusing way to do things.  which attach to call when.
      //
      // Really the agent's interface should be the primary one for changing the run state, but
      // the way I've put things together, it looks like the IDevices is primary.  In part this
      // is because the IDevice child provides an API for the device it represents.  Need a way
      // of distinguishing.  Perhaps just rename IDevice's attach()/detach() to __attach()/__detach()
      // or onAttach/onDetach...something to remind me it's a callback.
      //
      // also on_attach/on_detach only gets called for the owner, so the events have to be forwarded
      // to devices that share the agent.  This seems awkward :C

      eflag |= __scan_agent.attach(); //scanner.attach(); 
      eflag |= stage_.on_attach();

      std::string stackname = _config->file_prefix()+_config->stack_extension();
      file_series.ensurePathExists();   

      return eflag;  
    }
    
    unsigned int
    Microscope::on_detach(void)
    { 
      int eflag = 0; // 0 success, 1 failure
      eflag |= scanner._agent->detach(); //scanner.detach();
      eflag |= frame_averager._agent->detach();
      eflag |= pixel_averager._agent->detach();
      eflag |= inverter._agent->detach();
      eflag |= cast_to_i16._agent->detach();
      eflag |= wrap._agent->detach();
      eflag |= trash._agent->detach();
      eflag |= disk._agent->detach();

      eflag |= stage_.on_detach();
      return eflag;  
    }
    
    unsigned int Microscope::on_disarm()
    {
      unsigned int sts = 1; // success      
      sts &= scanner._agent->disarm();
      sts &= frame_averager._agent->disarm();
      sts &= pixel_averager._agent->disarm();
      sts &= inverter._agent->disarm();
      sts &= cast_to_i16._agent->disarm();
      sts &= wrap._agent->disarm();
      sts &= trash._agent->disarm();
      sts &= disk._agent->disarm();
      
      return sts;
    }
    
    const std::string Microscope::stack_filename()
    {     
      return file_series.getFullPath(_config->file_prefix(),_config->stack_extension());
    }  
    
    const std::string Microscope::config_filename()
    {     
      return file_series.getFullPath(_config->file_prefix(),_config->config_extension());
    }        

    const std::string Microscope::metadata_filename()
    {
      return file_series.getFullPath(_config->file_prefix(),_config->metadata_extension());
    }

    void Microscope::write_stack_metadata()
    {      
      //{ std::ofstream fout(config_filename(),std::ios::out|std::ios::trunc|std::ios::binary);
      //  get_config().SerializePartialToOstream(&fout);
      //}      
      { std::ofstream fout(config_filename().c_str(),std::ios::out|std::ios::trunc);
        std::string s;
        Config c = get_config();
        google::protobuf::TextFormat::PrintToString(c,&s);
        fout << s;
        //get_config().SerializePartialToOstream(&fout);
      }
      { float x,y,z;
        std::ofstream fout(metadata_filename().c_str(),std::ios::out|std::ios::trunc);
        fetch::cfg::data::Acquisition data;
        stage_.getPos(&x,&y,&z);
        data.set_x_mm(x);
        data.set_y_mm(y);
        data.set_z_mm(z);        
        std::string s;
        google::protobuf::TextFormat::PrintToString(data,&s);
        fout << s;        
        //get_config().SerializePartialToOstream(&fout);
      }
    }

    void Microscope::_set_config( Config IN *cfg )
    {
      scanner._set_config(cfg->mutable_scanner3d());
      pixel_averager._set_config(cfg->mutable_horizontal_downsample());
      frame_averager._set_config(cfg->mutable_frame_average());
      wrap._set_config(cfg->mutable_resonant_wrap());
      file_series._desc = cfg->mutable_file_series();
    }

    void Microscope::_set_config( const Config& cfg )
    {
      *_config=cfg;         // Copy
      _set_config(_config); // Update
    }

    void Microscope::onUpdate()
    {
      scanner.onUpdate();
      fov_.update(_config->fov());
      //stage_.onUpdate(); // only update the stage fov parameter to recompute the tiling.  probably, want to do that with another, more explicit, mechanism
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
      return cur;
    }

    void Microscope::__common_setup()
    {
      __self_agent._owner = this;
      stage_.setFOV(&fov_);
      Guarded_Assert(_agent->attach()==0);
      Guarded_Assert(_agent->arm(&interaction_task,this,INFINITE));
    }

    unsigned int Microscope::runPipeline()
    { int sts = 1;
      sts &= wrap._agent->run();
      sts &= cast_to_i16._agent->run();
      sts &= inverter._agent->run();
      sts &= frame_averager._agent->run();
      sts &= pixel_averager._agent->run();      
      return (sts!=1); // returns 1 on fail and 0 on success
    }
    
    unsigned int Microscope::stopPipeline()
    { int sts = 1;
      // These should block till channel's empty 
      sts &= wrap._agent->stop();
      sts &= cast_to_i16._agent->stop();
      sts &= inverter._agent->stop();
      sts &= frame_averager._agent->stop();
      sts &= pixel_averager._agent->stop();      
      return (sts!=1); // returns 1 on fail and 0 on success
    }

    ///////////////////////////////////////////////////////////////////////
    // FileSeries
    ///////////////////////////////////////////////////////////////////////
    
    FileSeries& FileSeries::inc( void )
    {
      int n = _desc->seriesno();   
      
      // reset series number when series path changes
      updateDate();                // get the current date
      std::string seriespath = _desc->root() + _desc->pathsep() + _desc->date();
      if(seriespath.compare(_lastpath)!=0)
      { _desc->set_seriesno(0);
        _lastpath = seriespath;
      } else
      { _desc->set_seriesno(n+1);
      }
      return *this;
    }

    const std::string FileSeries::getFullPath(const std::string& prefix, const std::string& ext)
    {
      char strSeriesNo[32];
      renderSeriesNo(strSeriesNo,sizeof(strSeriesNo));
      std::string seriespath = _desc->root() + _desc->pathsep() + _desc->date();

      std::string part2 = prefix;
      if(!part2.empty())
        part2 = "-" + prefix;

      return seriespath
        + _desc->pathsep()
        + strSeriesNo
        + _desc->pathsep()
        + strSeriesNo + part2 + ext; 
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
