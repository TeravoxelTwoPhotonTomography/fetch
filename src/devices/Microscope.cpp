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
#include "util\util-protobuf.h"
#include "util\util-mylib.h"

#define CHKJMP(expr,lbl) \
  if(!(expr)) \
  { warning("[MICROSCOPE] %s(%d): "ENDL"\tExpression: %s"ENDL"\tevaluated false."ENDL,__FILE__,__LINE__,#expr); \
    goto lbl; \
  }

#define DO_UNWARP
#include <ui/StageController.h>

namespace fetch
{

  bool operator==(const cfg::device::Microscope& a, const cfg::device::Microscope& b) {return equals(&a,&b);}
  bool operator!=(const cfg::device::Microscope& a, const cfg::device::Microscope& b) {return !(a==b);}

  namespace device {

    Microscope::Microscope()
      :IConfigurableDevice<Config>(&__self_agent)
      ,__self_agent("Microscope",NULL)
      ,__scan_agent("Scanner",&scanner)
      ,__io_agent("IO",&disk)
      ,__vibratome_agent("Vibratome",&vibratome_)
      ,scanner(&__scan_agent)
      ,stage_(&__self_agent)
      ,vibratome_(&__vibratome_agent)
      ,pmt_(&__self_agent)
      ,fov_(_config->fov())
      ,surface_probe_(&__scan_agent)
      ,disk(&__io_agent)
      ,pipeline()
      ,trip_detect(this)
      ,surface_finder()
      ,trash("Trash")
      ,_end_of_pipeline(0)
    {
      set_config(_config);
      pipeline.set_scan_rate_Hz(_config->scanner3d().scanner2d().frequency_hz());
      pipeline.set_sample_rate_MHz(scanner.get2d()->_digitizer.sample_rate_MHz());

      __common_setup();
    }

    Microscope::Microscope( const Config &cfg )
      :IConfigurableDevice<Config>(&__self_agent)
      ,__self_agent("Microscope",NULL)
      ,__scan_agent("Scanner",&scanner)
      ,__io_agent("IO",&disk)
      ,__vibratome_agent("Vibratome",&vibratome_)
      ,scanner(&__scan_agent)
      ,stage_(&__self_agent)
      ,vibratome_(&__vibratome_agent)
      ,pmt_(&__self_agent)
      ,fov_(cfg.fov())
      ,surface_probe_(&__scan_agent)
      ,disk(&__io_agent)
      ,pipeline()
      ,trip_detect(this)
      ,surface_finder()
      ,trash("Trash")
      ,file_series()
      ,_end_of_pipeline(0)
    {
      set_config(cfg);
      pipeline.set_scan_rate_Hz(_config->scanner3d().scanner2d().frequency_hz());
      pipeline.set_sample_rate_MHz(scanner.get2d()->_digitizer.sample_rate_MHz());
      __common_setup();
    }

    Microscope::Microscope(Config *cfg )
      :IConfigurableDevice<Config>(&__self_agent,cfg)
      ,__self_agent("Microscope",NULL)
      ,__scan_agent("Scanner",&scanner)
      ,__io_agent("IO",&disk)
      ,__vibratome_agent("Vibratome",&vibratome_)
      ,scanner(&__scan_agent,cfg->mutable_scanner3d())
      ,stage_(&__self_agent,cfg->mutable_stage())
      ,vibratome_(&__vibratome_agent,cfg->mutable_vibratome())
      ,pmt_(&__self_agent,cfg->mutable_pmt())
      ,fov_(cfg->fov())
      ,surface_probe_(&__scan_agent,cfg->mutable_surface_probe())
      ,pipeline(cfg->mutable_pipeline())
      ,trip_detect(this,cfg->mutable_trip_detect())
      ,surface_finder(cfg->mutable_surface_find())
      ,disk(&__io_agent)
      ,trash("Trash")
      ,file_series(cfg->mutable_file_series())
      ,_end_of_pipeline(0)
    {
      pipeline.set_scan_rate_Hz(_config->scanner3d().scanner2d().frequency_hz());
      pipeline.set_sample_rate_MHz(scanner.get2d()->_digitizer.sample_rate_MHz());
      __common_setup();
    }


    Microscope::~Microscope(void)
    {
      if(__scan_agent.detach()) warning("Microscope __scan_agent did not detach cleanly\r\n");
      if(__self_agent.detach()) warning("Microscope __self_agent did not detach cleanly\r\n");
      if(  __io_agent.detach()) warning("Microscope __io_agent did not detach cleanly\r\n");
      if(  __vibratome_agent.detach()) warning("Microscope __vibratome_agent did not detach cleanly\r\n");
    }

    unsigned int
    Microscope::on_attach(void)
    {
      // argh this is a confusing way to do things.  which attach to call when.
      //
      // on_attach/on_detach only gets called for the owner, so attach/detach events have to be forwarded
      // to devices that share the agent.  This seems awkward :C
      std::string stackname;

      CHKJMP(     __scan_agent.attach()==0,ESCAN);
      CHKJMP(        stage_.on_attach()==0,ESTAGE);
      CHKJMP(__vibratome_agent.attach()==0,EVIBRATOME);
      CHKJMP(          pmt_.on_attach()==0,EPMT);

      stackname = _config->file_prefix()+_config->stack_extension();

      return 0;   // success
EPMT:
      __vibratome_agent.detach();
EVIBRATOME:
      stage_.on_detach();
ESTAGE:
      __scan_agent.detach();
ESCAN:
      return 1;


    }

    unsigned int
    Microscope::on_detach(void)
    {
      int eflag = 0; // 0 success, 1 failure
      eflag |= scanner._agent->detach(); //scanner.detach();
      eflag |= pipeline._agent->detach();
      eflag |= trash._agent->detach();
      eflag |= disk._agent->detach();
      eflag |= stage_.on_detach();
      eflag |= vibratome_._agent->detach();
      eflag |= pmt_.on_detach();
      return eflag;
    }

    unsigned int Microscope::on_disarm()
    {
      unsigned int sts = 1; // success
      sts &= scanner._agent->disarm();
      sts &= pipeline._agent->disarm();
      sts &= trash._agent->disarm();
      sts &= disk._agent->disarm();
      sts &= vibratome_._agent->disarm();
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
      { std::ofstream fout(config_filename().c_str(),std::ios::out|std::ios::trunc);
        std::string s;
        Config c = get_config();
        google::protobuf::TextFormat::PrintToString(c,&s);
        fout << s;
      }
      { float x,y,z;
        std::ofstream fout(metadata_filename().c_str(),std::ios::out|std::ios::trunc);
        fetch::cfg::data::Acquisition data;
        stage_.getPos(&x,&y,&z);
        //StageTiling *t=stage_.tiling();
        //t->computeCursorPos()
        data.set_x_mm(x);
        data.set_y_mm(y);
        data.set_z_mm(z);
        
        #if 0 // one method -- the last cursor position
        {
            int x,y,z;
            stage_.tiling()->getCursorLatticePosition(&x,&y,&z);
            data.mutable_current_lattice_position()->set_x(x);
            data.mutable_current_lattice_position()->set_y(y);
            data.mutable_current_lattice_position()->set_z(z);
        }
        #else // other method -- project the current stage position to the lattice
        Vector3z r=stage_.getPosInLattice();
        data.mutable_current_lattice_position()->set_x(r(0));
        data.mutable_current_lattice_position()->set_y(r(1));
        data.mutable_current_lattice_position()->set_z(r(2));
        #endif
        
        data.set_cut_count(_cut_count);
        std::string s;
        google::protobuf::TextFormat::PrintToString(data,&s);
        fout << s;
      }
    }

    void Microscope::_set_config( Config IN *cfg )
    {
      scanner._set_config(cfg->mutable_scanner3d());
      pipeline._set_config(cfg->mutable_pipeline());
      vibratome_._set_config(cfg->mutable_vibratome());
      fov_.update(_config->fov());
      surface_probe_._set_config(cfg->mutable_surface_probe());
      stage_._set_config(cfg->mutable_stage());
      pmt_._set_config(cfg->mutable_pmt());

      pipeline.set_scan_rate_Hz(_config->scanner3d().scanner2d().frequency_hz());
      pipeline.set_sample_rate_MHz(scanner.get2d()->_digitizer.sample_rate_MHz());
    }

    void Microscope::_set_config( const Config& cfg )
    {
      *_config=cfg;         // Copy
      _set_config(_config); // Update
    }

    void Microscope::onUpdate()
    {
      scanner.onUpdate();
      vibratome_.onUpdate();
      fov_.update(_config->fov());
      stage_.setFOV(&fov_);
      surface_probe_.onUpdate();
      pipeline.set_scan_rate_Hz(_config->scanner3d().scanner2d().frequency_hz());
      pipeline.set_sample_rate_MHz(scanner.get2d()->_digitizer.sample_rate_MHz());

      file_series.updateDesc(_config->mutable_file_series());

      // update microscope's run state based on sub-agents
      // require scan agent and vibratome agent to be attached
      if( __self_agent.is_attached() && !(__scan_agent.is_attached() && __vibratome_agent.is_attached()))
        __self_agent.detach();
    }

    IDevice* Microscope::configPipeline()
    {
      //Assemble pipeline here
      IDevice *cur;
      cur = &scanner;
      cur =  pipeline.apply(cur);
      cur =  trip_detect.apply(cur);
      _end_of_pipeline=cur;
      return cur;
    }

    static void load_cut_count(int* cut_count) 
    {
        LONG ecode;
        DWORD nbytes=sizeof(*cut_count);
        const char *path[]={"Software","Howard Hughes Medical Institute","Fetch","Microscope"};
        HKEY key=HKEY_CURRENT_USER;
        for(int i=0;i<_countof(path);++i)
            RegCreateKey(key,path[i],&key);
        
        ecode=RegQueryValueEx(key,"cut_count",0,0,(BYTE*)cut_count,&nbytes);
        if(ecode==ERROR_FILE_NOT_FOUND) 
        {
            *cut_count=0;
            Guarded_Assert_WinErr(ERROR_SUCCESS==(
                RegSetValueEx(key,"cut_count",0,REG_DWORD,
                    (const BYTE*)cut_count,nbytes)));
        } else
        {
            Guarded_Assert_WinErr(ecode==ERROR_SUCCESS);
        }
    }

    void Microscope::__common_setup()
    {
      __self_agent._owner = this;
      stage_.setFOV(&fov_);
      CHKJMP(_agent->attach()==0,Error);
      CHKJMP(_agent->arm(&interaction_task,this,INFINITE)==0,Error);
      load_cut_count(&this->_cut_count);
    Error:
      return;
    }

    unsigned int Microscope::runPipeline()
    { int sts = 1;
      transaction_lock();
      sts &= pipeline._agent->run();
      sts &= trip_detect._agent->run();
      transaction_unlock();
      return (sts!=1); // returns 1 on fail and 0 on success
    }

    unsigned int Microscope::stopPipeline()
    { int sts = 1;
      transaction_lock();
      sts &= pipeline._agent->stop();
      sts &= trip_detect._agent->stop();
      transaction_unlock();
      return (sts!=1); // returns 1 on fail and 0 on success
    }

#define TRY(e) \
    if(!(e))                                         \
    do { warning("Expression evalutated as false"ENDL \
              "\t%s(%d)"ENDL                         \
              "\t%s"ENDL,                            \
              __FILE__,__LINE__,#e);                 \
      goto Error;                                    \
    } while(0)

    /**
    Configures the microscope to take a single snapshot at a depth specified by \a dz_um.
    Uses the the fetch::task::scanner:ScanStack task that is reconfigured to acquire
    just enough frames so that one frame is emitted from the end of the pipeline.

    The machine state is restored after calling this function so that temporary changes to the
    config and scanner task are transparent.

    This synchronously executes.  It should not be called from the GUI thread.  It is most
    appropriate to use the Microscope agent's thread.

    This ends up allocating a frame for each snapshot and using a copy to form the returned
    mylib::Array.

    \returns NULL on failure, otherwise a Mylib::Array*.  The caller is responsible for
             freeing the array.
    */
    mylib::Array* Microscope::snapshot(float dz_um, unsigned timeout_ms)
    { Config cfg(get_config());
      const Config original(get_config());
      Chan* c=0;
      Frame *frm=0;
      mylib::Array *ret=0;
      Task* oldtask;
      static task::scanner::ScanStack<u16> scan;
      transaction_lock();
      // 1. Set up the stack acquisition
      { int nframe = cfg.pipeline().frame_average_count(); //cfg.frame_average().ntimes();
        float step = 0.1/(float)nframe;
        // z-scan range is inclusive
        cfg.mutable_scanner3d()->mutable_zpiezo()->set_um_min(dz_um);
        cfg.mutable_scanner3d()->mutable_zpiezo()->set_um_max(dz_um+(nframe-1)*step); //ensure n frames are aquired
        cfg.mutable_scanner3d()->mutable_zpiezo()->set_um_step(step);
      }
      scanner.set_config(cfg.scanner3d());              // commit config
      Chan *out = configPipeline()->_out->contents[0];  // pipeline - don't connect end to anything, as we'll read produced data here.
      TRY(c=Chan_Open(out,CHAN_READ)); // open the output channel before starting to ensure we get the data

      // 2. Start the acquisition
      oldtask = __scan_agent._task;
      TRY(0==__scan_agent.disarm(timeout_ms));
      TRY(0==__scan_agent.arm(&scan,&scanner));
      TRY(0==runPipeline());
      Chan_Wait_For_Writer_Count(c,1);
      TRY(__scan_agent.run());

      // 3. Wait for result
      TRY(frm=(Frame*) Chan_Token_Buffer_Alloc(c));
      //TRY(CHAN_SUCCESS(Chan_Next(c,(void**)&frm,Chan_Buffer_Size_Bytes(c))));
      TRY(CHAN_SUCCESS(Chan_Next_Timed(c,(void**)&frm,Chan_Buffer_Size_Bytes(c),timeout_ms)));
      { mylib::Array dummy;
        mylib::Dimn_Type dims[3];
        mylib::castFetchFrameToDummyArray(&dummy,frm,dims);
        ret=mylib::Copy_Array(&dummy);
      }

Finalize:

      TRY(__scan_agent.stop());
      TRY(0==__scan_agent.disarm(timeout_ms));
      if(frm) Chan_Token_Buffer_Free(frm);
      if(c) Chan_Close(c);
      stopPipeline(); //- redundant?
      scanner.set_config(original.scanner3d());
      __scan_agent._task=oldtask;
      transaction_unlock();
      return ret;
Error:
      ret=NULL;
      goto Finalize;
    }

    int Microscope::updateFovFromStackDepth(int nowait)
    { Config c = get_config();
      float s,t;
      f64 ummin,ummax,umstep;
      scanner._zpiezo.getScanRange(&ummin,&ummax,&umstep);
      s = scanner.zpiezo()->getMax() - scanner.zpiezo()->getMin();
      t = vibratome()->thickness_um();
      c.mutable_fov()->set_z_size_um( s );
      c.mutable_fov()->set_z_overlap_um( s-t );
      if(nowait)
        TRY( set_config_nowait(c) );
      else
        set_config(c);

      return (s-t)>0.0;
Error:
      return 0;
    }

    int Microscope::updateStackDepthFromFov(int nowait)
    { Config c = get_config();
      float s,t,o;
      o = c.fov().z_overlap_um();
      t = vibratome()->thickness_um();
      s = t+o;

      // - Really, I want one big atomic update of the microscope state.
      //   and I'll get it this way.
      cfg::device::ZPiezo *z = c.mutable_scanner3d()->mutable_zpiezo();
      z->set_um_min(0.0);
      z->set_um_max(s);
      c.mutable_fov()->set_z_size_um( s );
      if(nowait)
        TRY( set_config_nowait(c) );
      else
        set_config(c);

      return (s-t)>0.0;
Error:
      return 0;
    }

    ///////////////////////////////////////////////////////////////////////
    // FileSeries
    ///////////////////////////////////////////////////////////////////////

#if 0
#define VALIDATE if(!_is_valid) {warning("(%s:%d) - Invalid location for file series."ENDL,__FILE__,__LINE__);}
#else
#define VALIDATE
#endif


    FileSeries& FileSeries::inc( void )
    {
      VALIDATE;
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
      _prev.set_seriesno(_desc->seriesno());
      notify();
      return *this;
    }

    const std::string FileSeries::getFullPath(const std::string& prefix, const std::string& ext)
    {
      VALIDATE;
      char strSeriesNo[32];
      char two[3]={0};
      renderSeriesNo(strSeriesNo,sizeof(strSeriesNo));
      std::string seriespath = _desc->root() + _desc->pathsep() + _desc->date();
      two[0]=strSeriesNo[0];
      two[1]=strSeriesNo[1];

      std::string part2 = prefix;
      if(!part2.empty())
        part2 = "-" + prefix;

      return seriespath
        + _desc->pathsep()
        + two
        + _desc->pathsep()
        + strSeriesNo
        + _desc->pathsep()
        + strSeriesNo + part2 + ext;
    }

    const std::string FileSeries::getPath()
    {
      VALIDATE;
      char strSeriesNo[32];
      char two[3]={0};
      renderSeriesNo(strSeriesNo,sizeof(strSeriesNo));
      std::string seriespath = _desc->root() + _desc->pathsep() + _desc->date();
      two[0]=strSeriesNo[0];
      two[1]=strSeriesNo[1];

      return seriespath
        + _desc->pathsep()
        + two
        + _desc->pathsep()
        + strSeriesNo
        + _desc->pathsep();
    }

    void FileSeries::updateDate( void )
    {
      time_t clock = time(NULL);
      struct tm *t = localtime(&clock);
      char datestr[] = "0000-00-00";
      sprintf_s(datestr,sizeof(datestr),"%04d-%02d-%02d",t->tm_year+1900,t->tm_mon+1,t->tm_mday);
      _desc->set_date(datestr);
      _prev.set_date(_desc->date());
    }

    bool FileSeries::updateDesc(cfg::FileSeries *desc)
    {
      if(_desc)
        desc->set_seriesno(_prev.seriesno()); // keep old series no
      _desc = desc;
      _prev.CopyFrom(*_desc);
      updateDate();
      //ensurePathExists();
      notify();
      return is_valid();
    }


    bool FileSeries::ensurePathExists()
    {
      std::string s,t,u;
      char strSeriesNo[32]={0},two[3]={0};

      renderSeriesNo(strSeriesNo,sizeof(strSeriesNo));
      two[0]=strSeriesNo[0];
      two[1]=strSeriesNo[1];
      updateDate();

      tryCreateDirectory(_desc->root().c_str(), "root path", "");

      s = _desc->root()+_desc->pathsep()+_desc->date();
      tryCreateDirectory(s.c_str(), "date path", _desc->root().c_str());

      t = s + _desc->pathsep()+two;
      tryCreateDirectory(t.c_str(), "series path prefix", s.c_str());

      u = t + _desc->pathsep()+strSeriesNo;
      tryCreateDirectory(u.c_str(), "series path", t.c_str());

      return is_valid();
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
      _is_valid = true;
      if(!CreateDirectory(path,NULL/*default security*/))
      { DWORD err;
        switch(err=GetLastError())
        {
        case ERROR_ALREADY_EXISTS: /*ignore*/
          break;
        case ERROR_PATH_NOT_FOUND:
        case ERROR_NOT_READY:
          warning("[FileSeries] %s(%d)"ENDL"\tCould not create %s:"ENDL"\t%s"ENDL,__FILE__,__LINE__,description,path); // [ ] TODO chage this to a warning
          _is_valid = false;
          break;
        default:
          _is_valid = false;
          warning("[FileSeries] %s(%d)"ENDL"\tUnexpected error returned after call to CreateDirectory()"ENDL"\tFor Path: %s"ENDL,__FILE__,__LINE__,path);
          ReportLastWindowsError();
        }
      }
    }

    void
      FileSeries::
      notify()
    { TListeners::iterator i;
      std::string path = getPath();
      for(i=_listeners.begin();i!=_listeners.end();++i)
        (*i)->update(path);
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
