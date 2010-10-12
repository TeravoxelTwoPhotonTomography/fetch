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

#include "StdAfx.h"

#include "Scanner3D.h"
#include "Scanner2D.h"


#define DAQWRN( expr )        (Guarded_DAQmx( (expr), #expr, warning))
#define DAQERR( expr )        (Guarded_DAQmx( (expr), #expr, error  ))
#define DAQJMP( expr )        goto_if_fail( 0==DAQWRN(expr), Error)

namespace fetch
{
  namespace device
  {

    Scanner3D::Scanner3D( Agent *agent )
      :IConfigurableDevice<Config>(agent)
      ,_scanner2d(agent)
      ,_zpiezo(agent)
    {
      set_config(_config);
    }

    Scanner3D::Scanner3D( Agent *agent, Config *cfg )
      :IConfigurableDevice<Config>(agent,cfg)
      ,_scanner2d(agent,cfg->mutable_scanner2d())
      ,_zpiezo(agent,cfg->mutable_zpiezo())
    {

    }

    unsigned int Scanner3D::attach()
    {
      unsigned int eflag = 0; //success = 0, error = 1
      return_val_if(eflag |= _scanner2d.attach()  ,eflag);
      return_val_if(eflag |= _zpiezo.attach()     ,eflag);
      return eflag;
    }

    unsigned int Scanner3D::detach()
    {
      unsigned int eflag = 0; //success = 0, error = 1
      return_val_if(eflag |= _scanner2d.detach()  ,eflag);
      return_val_if(eflag |= _zpiezo.detach()     ,eflag);
      return eflag;
    }

    void Scanner3D::_set_config( Config IN *cfg )
    {
      _scanner2d._set_config(cfg->mutable_scanner2d());
      _zpiezo._set_config(cfg->mutable_zpiezo());
      _config = cfg;
    }

    void Scanner3D::_set_config( const Config& cfg )
    {
      *_config=cfg;
      _zpiezo.setKind(_config->zpiezo().kind());
    }

    void Scanner3D::onConfig()
    {
      float64 nscans       = _config->scanner2d().nscans(),
              scan_freq_Hz = _config->scanner2d().frequency_hz();
      IDAQChannel *chans[] = {
        _scanner2d._LSM.physicalChannel(),
        _scanner2d._pockels.physicalChannel(),
        _zpiezo.physicalChannel()
      };
      _scanner2d._daq.setupCLK(nscans,scan_freq_Hz);
      _scanner2d._daq.setupAO(nscans,scan_freq_Hz);
      _scanner2d._daq.setupAOChannels(nscans,scan_freq_Hz,-10,10,chans,3);

      _scanner2d._shutter.Shut();
      _scanner2d._digitizer.setup((int)nscans,scan_freq_Hz,_scanner2d._config->line_duty_cycle());
    }

    void Scanner3D::writeAO()
    {
      _scanner2d.writeAO();
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
      int N = _scanner2d._daq.samplesPerRecordAO();
      transaction_lock();
      vector_f64_request(_ao_workspace,2*N-1/*max index*/);
      f64 *m = _ao_workspace->contents,
          *p = m+N,
          *z = p+N;
      _scanner2d._LSM.computeSawtooth(m,N);
      _scanner2d._pockels.computeVerticalBlankWaveform(p,N);
      _zpiezo.computeConstWaveform(z_um,z,N);
      transaction_unlock();
    }

    void Scanner3D::generateAORampZ()
    {
      generateAORampZ(_config->zref_um());
    }

    void Scanner3D::generateAORampZ( float z_um )
    {
      int N = _scanner2d._daq.samplesPerRecordAO();
      transaction_lock();
      vector_f64_request(_ao_workspace,2*N-1/*max index*/);
      f64 *m = _ao_workspace->contents,
        *p = m+N,
        *z = p+N;
      _scanner2d._LSM.computeSawtooth(m,N);
      _scanner2d._pockels.computeVerticalBlankWaveform(p,N);
      _zpiezo.computeRampWaveform(z_um,z,N);
      transaction_unlock();
    }

  }
}
