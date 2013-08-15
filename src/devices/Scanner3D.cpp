/*
 * Scanner3D.cpp
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

#include "Scanner3D.h"
#include "Scanner2D.h"

#define countof(e) (sizeof(e)/sizeof(*e))

#define DAQWRN( expr )        (Guarded_DAQmx( (expr), #expr, __FILE__, __LINE__, warning))
#define DAQERR( expr )        (Guarded_DAQmx( (expr), #expr, __FILE__, __LINE__, error  ))
#define DAQJMP( expr )        goto_if_fail( 0==DAQWRN(expr), Error)

#define CHKJMP(expr,lbl) \
  if(!(expr)) \
    { warning("%s(%d):"ENDL"\tExpression: %s"ENDL"\tEvaluated as false."ENDL,__FILE__,__LINE__,#expr); \
      goto lbl; \
    }

namespace fetch
{

  bool operator==(const cfg::device::Scanner3D& a, const cfg::device::Scanner3D& b)         {return equals(&a,&b);}
  bool operator!=(const cfg::device::Scanner3D& a, const cfg::device::Scanner3D& b)         {return !(a==b);}

  namespace device
  {

    Scanner3D::Scanner3D( Agent *agent )
      :IConfigurableDevice<Config>(agent)
      ,_scanner2d(agent)
      ,_zpiezo(agent)
    {
      _set_config(_config);
      __common_setup();
    }

    Scanner3D::Scanner3D( Agent *agent, Config *cfg )
      :IConfigurableDevice<Config>(agent,cfg)
      ,_scanner2d(agent,cfg->mutable_scanner2d())
      ,_zpiezo(agent,cfg->mutable_zpiezo())
    {
      __common_setup();
    }

    unsigned int Scanner3D::on_attach()
    {
      CHKJMP(_scanner2d.on_attach()==0  ,ESCAN2D);
      CHKJMP(   _zpiezo.on_attach()==0  ,EZPIEZO);

      if(!_out)
      {
        // Reference the video output channel
        int n = _scanner2d._out->nelem;
        _out=vector_PCHAN_alloc(n);
        //Guarded_Assert(_out->contents[0] = Chan_Open(_scanner2d.getVideoChannel(),CHAN_NONE) );
        for(int i=0;i<n;++i)
          Guarded_Assert(_out->contents[i] = Chan_Open(_scanner2d._out->contents[i],CHAN_NONE));

      }
      //
      return 0;
      // on error, try to detach attached sub-devices
EZPIEZO:
      _scanner2d.on_detach();
ESCAN2D:
      return 1;
    }

    unsigned int Scanner3D::on_detach()
    {
      unsigned int eflag = 0; //success = 0, error = 1
      return_val_if(eflag |= _scanner2d.on_detach()  ,eflag);
      return_val_if(eflag |= _zpiezo.on_detach()     ,eflag);
      return eflag;
    }

    /// Returns 0 on success, nonzero otherwise
    unsigned int Scanner3D::on_disarm()
    { int ecode=0;
      ecode |= _zpiezo.on_disarm();
      ecode |= _scanner2d.on_disarm();
      return ecode;
    }

    void Scanner3D::_set_config( Config IN *cfg )
    {
      _scanner2d._set_config(cfg->mutable_scanner2d());
      _zpiezo._set_config(cfg->mutable_zpiezo());
      _config = cfg;
    }

    // For some reason I don't want this to do a full config
    // The problem is Microscope::snapshot() which calls this function
    // and should probably use update or something instead.
    void Scanner3D::_set_config( const Config& cfg )
    {
      *_config=cfg;
      //_set_config(_config);
      _zpiezo.setKind(_config->zpiezo().kind());
    }

    int Scanner3D::onConfigTask()
    {
      float64 nscans       = _config->scanner2d().nscans(),
              scan_freq_Hz = _config->scanner2d().frequency_hz();
      int isok=1;
      IDAQPhysicalChannel *chans[] = {
        _scanner2d._LSM.physicalChannel(),
        _scanner2d._pockels1.physicalChannel(),
        _scanner2d._pockels2.physicalChannel(),
        _zpiezo.physicalChannel()
      };
      _scanner2d._daq.setupCLK(nscans,scan_freq_Hz);
      _scanner2d._daq.setupAO(nscans,scan_freq_Hz);
      _scanner2d._daq.setupAOChannels(nscans,scan_freq_Hz,-10,10,chans,countof(chans));

      _scanner2d._shutter.Shut();
      isok &= _scanner2d._digitizer.setup((int)nscans,scan_freq_Hz,_scanner2d._config->line_duty_cycle());
      return isok;
    }

    int Scanner3D::writeAO()
    {
      return _scanner2d._daq.writeAO(_ao_workspace->contents);
//      _scanner2d.writeAO(); // !! don't do this!  It'll write the scanner2d's ao buffer which is the wrong one.
    }

    void Scanner3D::generateAO()
    {
      generateAOConstZ();
    }

    void Scanner3D::generateAOConstZ()
    {
      generateAOConstZ(_config->zref_um());
    }

    void Scanner3D::generateAOConstZ( float z_um )
    {
      int f, N;
      transaction_lock();
      N = _scanner2d._daq.samplesPerRecordAO();
      f = _scanner2d._daq.flybackSampleIndex(_config->scanner2d().nscans());
      vector_f64_request(_ao_workspace,4*N-1/*max index*/);
      f64 *m  = _ao_workspace->contents,
          *p1 = m+N,
          *p2 =p1+N,
          *z  =p2+N;
      _scanner2d._LSM.computeSawtooth(m,f,N);
      _scanner2d._pockels1.computeVerticalBlankWaveform(p1,f,N);
      _scanner2d._pockels2.computeVerticalBlankWaveform(p2,f,N);
      _zpiezo.computeConstWaveform(z_um,z,f,N);
      transaction_unlock();
    }

    void Scanner3D::generateAORampZ()
    {
      generateAORampZ(_config->zref_um());
    }

    void Scanner3D::generateAORampZ( float z_um )
    {
      int N,f;
      transaction_lock();
      N = _scanner2d._daq.samplesPerRecordAO();
      f = _scanner2d._daq.flybackSampleIndex(_config->scanner2d().nscans());
      vector_f64_request(_ao_workspace,4*N-1/*max index*/);
      f64 *m  = _ao_workspace->contents,
          *p1 = m+N,
          *p2 =p1+N,
          *z  =p2+N;
      _scanner2d._LSM.computeSawtooth(m,f,N);
      _scanner2d._pockels1.computeVerticalBlankWaveform(p1,f,N);
      _scanner2d._pockels2.computeVerticalBlankWaveform(p2,f,N);
      _zpiezo.computeRampWaveform(z_um,z,f,N);
      transaction_unlock();
#if 0
      vector_f64_dump(_ao_workspace,"Scanner3D_generateAORampZ.f64");
#endif
    }

    void Scanner3D::__common_setup()
    {
      _ao_workspace = vector_f64_alloc(_scanner2d._daq.samplesPerRecordAO()*3);
    }

    int Scanner3D::writeLastAOSample()
    { int N = _scanner2d._daq.samplesPerRecordAO();
      f64 *m = _ao_workspace->contents;
      float64 last[] = {m[N-1],m[2*N-1],m[3*N-1],m[4*N-1]};
      return _scanner2d._daq.writeOneToAO(last);
    }


  }
}
