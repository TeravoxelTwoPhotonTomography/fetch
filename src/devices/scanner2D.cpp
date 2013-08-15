/*
 * Scanner2D.cpp
 *
 *  Created on: Apr 19, 2010
 *      Author: Nathan Clack <clackn@janelia.hhmi.org>
 */
/*
 * Copyright 2010 Howard Hughes Medical Institute.
 * All rights reserved.
 * Use is subject to Janelia Farm Research Campus Software Copyright 1.1
 * license terms (http://license.janelia.org/license/jfrc_copyright_1_1.html).
 */
#include "common.h"
#include "scanner2D.h"
#include "frame.h"
#include <algorithm>    // std::for_each

#define countof(e) (sizeof(e)/sizeof(*e))

#define DIGWRN( expr )  (niscope_chk( vi, expr, #expr, __FILE__, __LINE__, warning ))
#define DIGERR( expr )  (niscope_chk( vi, expr, #expr __FILE__, __LINE__, error   ))
#define DIGJMP( expr )  goto_if_fail(VI_SUCCESS == niscope_chk( vi, expr, #expr, warning ), Error)
#define DAQWRN( expr )  (Guarded_DAQmx( (expr), #expr, __FILE__, __LINE__, warning))
#define DAQERR( expr )  (Guarded_DAQmx( (expr), #expr, __FILE__, __LINE__, error  ))
#define DAQJMP( expr )  goto_if_fail( 0==DAQWRN(expr), Error)

#define CHKJMP(expr,lbl) \
  if(!(expr)) \
    { warning("%s(%d):"ENDL"\tExpression: %s"ENDL"\tEvaluated as false."ENDL,__FILE__,__LINE__,#expr); \
      goto lbl; \
    }

namespace fetch
{

  bool operator==(const cfg::device::Scanner2D& a, const cfg::device::Scanner2D& b)         {return equals(&a,&b);}
  bool operator!=(const cfg::device::Scanner2D& a, const cfg::device::Scanner2D& b)         {return !(a==b);}


  namespace device
  {


    Scanner2D::Scanner2D( Agent *agent )
      :IConfigurableDevice<Config>(agent)
      ,_digitizer(agent)
      ,_daq(agent)
      ,_shutter(agent)
      ,_LSM(agent)
      ,_pockels1(agent)
      ,_pockels2(agent)
    { 
      Pockels *ps[]={&_pockels1,&_pockels2};
      _pockels_map[cfg::device::Pockels::Chameleon]=ps[0];
      _pockels_map[cfg::device::Pockels::Fianium]=ps[1];
      _set_config(_config);
      __common_setup();
    }

    Scanner2D::Scanner2D( Agent *agent, Config *cfg )
      :IConfigurableDevice<Config>(agent,cfg)
      ,_digitizer(agent,cfg->mutable_digitizer())
      ,_daq(agent,cfg->mutable_daq())
      ,_shutter(agent,cfg->mutable_shutter())
      ,_LSM(agent,cfg->mutable_linear_scan_mirror())
      ,_pockels1(agent)
      ,_pockels2(agent)
    {      
      Pockels *ps[]={&_pockels1,&_pockels2};
      _pockels_map[cfg::device::Pockels::Chameleon]=ps[0];
      _pockels_map[cfg::device::Pockels::Fianium]=ps[1];
      for(int i=0;i<cfg->pockels_size();++i)
      { ps[i]->_set_config(cfg->mutable_pockels(i));
      }
      
      __common_setup();
    }

    unsigned int Scanner2D::on_attach()
    
{
      CHKJMP(_shutter.on_attach()==0   ,ESHUTTER);
      CHKJMP(_digitizer.on_attach()==0 ,EDIGITIZER);
      CHKJMP(_daq.on_attach()==0       ,EDAQ);
      CHKJMP(_LSM.on_attach()==0       ,ELSM);
      CHKJMP(_pockels1.on_attach()==0  ,EPOCKELS1);
      CHKJMP(_pockels2.on_attach()==0  ,EPOCKELS2);

      if(!_out)
      {
        // Guess for how the output will be sized for allocation.
        // [aside] this is gross and feels gross
        FrmFmt fmt(
          (u16)_digitizer.record_size(_config->frequency_hz(),_config->line_duty_cycle()),
          (u16)_config->nscans(),
          (u8)_digitizer.nchan(),
          id_u16);
        int n;
        size_t nout=1;
        size_t *sizes=0;
        _digitizer.aux_info(&n,&sizes);
        {
          nout+=n;
          size_t *nbuf, *nbytes;
          nbuf   = (size_t*)Guarded_Malloc(sizeof(size_t)*nout,"Scanner2D::on_attach()");
          nbytes = (size_t*)Guarded_Malloc(sizeof(size_t)*nout,"Scanner2D::on_attach()");
          for(size_t i=0;i<nout;++i)
            nbuf[i] = 4;
          nbytes[0] = fmt.size_bytes();
          if(n)
            memcpy(nbytes+1,sizes,sizeof(size_t)*n);

          _alloc_qs(&_out,nout,nbuf,nbytes);

          if(sizes)  free(sizes);
          if(nbytes) free(nbytes);
          if(nbuf)   free(nbuf);
        }
      }

      return 0; // success
      // On error, try to detached successfully attached subdevices.
EPOCKELS2:
      _pockels1.on_detach();
EPOCKELS1:
      _LSM.on_detach();
ELSM:
      _daq.on_detach();
EDAQ:
      _digitizer.on_detach();
EDIGITIZER:
      _shutter.on_detach();
ESHUTTER:
      return 1; // error flag
    }

    unsigned int Scanner2D::on_detach()
    {
      unsigned int eflag = 0; //success = 0, error = 1
      return_val_if(eflag |= _shutter.on_detach()  ,eflag);
      return_val_if(eflag |= _digitizer.on_detach(),eflag);
      return_val_if(eflag |= _daq.on_detach()      ,eflag);
      return_val_if(eflag |= _LSM.on_detach()      ,eflag);
      return_val_if(eflag |= _pockels1.on_detach()  ,eflag);
      return_val_if(eflag |= _pockels2.on_detach()  ,eflag);
      return eflag;
    }
    /// \returns 0 on success, 1 on failure
    unsigned int Scanner2D::on_disarm()
    { unsigned eflag;
      eflag |= _shutter.on_disarm();
      eflag |= _digitizer.on_disarm();
      eflag |= _daq.on_disarm();
      eflag |= _LSM.on_disarm();
      eflag |= _pockels1.on_disarm();
      eflag |= _pockels2.on_disarm();
      return eflag;
    }
    void Scanner2D::_set_config( Config IN *cfg )
    {
      // Changing where the Config data is referenced
      // Propagate the change to sub-devices.
      _digitizer._set_config(cfg->mutable_digitizer());
      _daq._set_config(cfg->mutable_daq());
      _shutter._set_config(cfg->mutable_shutter());
      _LSM._set_config(cfg->mutable_linear_scan_mirror());

      for(int i=0;i<cfg->pockels_size();++i) // At this point the mapping is required to exist
        _pockels_map.at(cfg->pockels(i).laser())->_set_config(cfg->mutable_pockels(i));

      _config = cfg;
    }

    void Scanner2D::_set_config( const Config& cfg )
    {
      // Copies the input config.
      // polymorphic devices need to get notified in case their "kind" changed
      *_config = cfg;
      _digitizer.setKind(_config->digitizer().kind());
      _daq.setKind(_config->daq().kind());
      _shutter.setKind(_config->shutter().kind());
      _LSM.setKind(_config->linear_scan_mirror().kind());
      for(int i=0;i<_config->pockels_size();++i) // At this point the mapping is required to exist
        _pockels_map.at(_config->pockels(i).laser())
                    ->setKind(_config->pockels(i).kind());
    }

    int Scanner2D::onConfigTask()
    {
      float64 nscans       = _config->nscans(),
              scan_freq_Hz = _config->frequency_hz();
      int isok=1;
      IDAQPhysicalChannel *chans[] = {
        _LSM.physicalChannel(),
        _pockels1.physicalChannel(),
        _pockels2.physicalChannel()
      };
      _daq.setupCLK(nscans,scan_freq_Hz);
      _daq.setupAO(nscans,scan_freq_Hz);
      _daq.setupAOChannels(nscans,scan_freq_Hz,-10,10,chans,countof(chans));

      _shutter.Shut();

      isok &= _digitizer.setup((int)nscans,scan_freq_Hz,_config->line_duty_cycle());
      return isok;
    }

    void Scanner2D::generateAO()
    {
      int N,f;
      transaction_lock();
      N = _daq.samplesPerRecordAO();
      f = _daq.flybackSampleIndex(_config->nscans());
      vector_f64_request(_ao_workspace,3*N-1/*max index*/);
      f64 *m = _ao_workspace->contents,
          *p1= m+N,
          *p2=p1+N;
      _LSM.computeSawtooth(m,f,N);
      _pockels1.computeVerticalBlankWaveform(p1,f,N);
      _pockels2.computeVerticalBlankWaveform(p2,f,N);
      transaction_unlock();
    }

    void Scanner2D::__common_setup()
    {
      _ao_workspace = vector_f64_alloc(_daq.samplesPerRecordAO()*3);

      // setup name mapping
      { const char* names[]={"Chameleon","chameleon","800nm"};
        for(int i=0;i<countof(names);++i)
          _laser_line_map[names[i]]=cfg::device::Pockels::Chameleon;
      }

      { const char* names[]={"Fianium","fianium","1064nm"};
        for(int i=0;i<countof(names);++i)
          _laser_line_map[names[i]]=cfg::device::Pockels::Fianium;
      }
    }

    int Scanner2D::writeAO()
    {
      return _daq.writeAO(_ao_workspace->contents);
    }

    int Scanner2D::writeLastAOSample()
    { int N = _daq.samplesPerRecordAO();
      f64 *m = _ao_workspace->contents;
      float64 last[] = {m[N-1],m[2*N-1],m[3*N-1]};
      return _daq.writeOneToAO(last);
    }

    Pockels* Scanner2D::pockels(const std::string& name)
    {
      try
      { return _pockels_map.at(_laser_line_map.at(name));
      } catch(std::out_of_range& err)
      { warning("%s(%d):"ENDL
                "\tCould not find pockels cell by name."ENDL
                "\tName: %s"ENDL,
                __FILE__,__LINE__,name.c_str());
      }
      return NULL;
    }

    Pockels* Scanner2D::pockels(unsigned idx)
    { Pockels *ps[]={&_pockels1,&_pockels2};
      if(idx<countof(ps))
        return ps[idx];
      return NULL;
    }

    Pockels* Scanner2D::pockels(cfg::device::Pockels::LaserLineIdentifier id)
    { try
      { return _pockels_map.at(id);
      } catch(std::out_of_range& err)
      { error("%s(%d):"ENDL
                "\tCould not find pockels cell by LaserLineIdentifier."ENDL
                "\tID: %d"ENDL,
                __FILE__,__LINE__,(int)id);
      }
      return NULL;
    }


  }
}

