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
#include "stdafx.h"
#include "scanner2D.h"
#include "../frame.h"

#define DAQWRN( expr )        (Guarded_DAQmx( (expr), #expr, warning))
#define DAQERR( expr )        (Guarded_DAQmx( (expr), #expr, error  ))
#define DAQJMP( expr )        goto_if_fail( 0==DAQWRN(expr), Error)

namespace fetch
{

  namespace device
  {

    Scanner2D::Scanner2D() :
      config(), ao(NULL), clk(NULL), ao_workspace(NULL)
    {
      // - ao_workspace is used to generate the data used for analog output during
      //   a frame. Need 2*nsamples b.c. there are two ao channels (the linear
      //   scan mirror and the pockels cell)
      ao_workspace = vector_f64_alloc(config.nsamples * 2);
    }

    Scanner2D::~Scanner2D()
    { if (!this->detach()>0)
        warning("Could not cleanly detach Scanner2D. (addr = 0x%p)\r\n",this);
      vector_f64_free(ao_workspace);
    }

    ViInt32
    Scanner2D::_compute_record_size(void)
    {
      double duty = this->config.line_duty_cycle,
             rate = ((Digitizer*) (this))->config.sample_rate,
             freq = this->config.frequency_Hz;
      return (ViInt32)(duty * rate / freq);
    }

    Frame_With_Interleaved_Lines
    Scanner2D::_describe_frame()
    {
      ViInt32 samples_per_scan = _compute_record_size();
      Frame_With_Interleaved_Lines
          format((u16)(samples_per_scan),
                 config.nscans,
                 (u8) (((Digitizer*) (this))->config.num_channels),
                 id_i16);
      Guarded_Assert(format.nchan > 0);
      Guarded_Assert(format.height > 0);
      Guarded_Assert(format.width > 0);
      return format;
    }

    unsigned int
    Scanner2D::detach(void)
    {
      unsigned int status = 0; // success 0, error 1
      if (!this->disarm(SCANNER2D_DEFAULT_TIMEOUT))
        warning("Could not cleanly disarm Scanner2D.\r\n");
      status |= this->Shutter::detach();
      status |= this->Digitizer::detach();
      this->lock();
      if (clk)
      {
        debug("Scanner2D: Attempting to detach DAQ CLK channel. handle: 0x%p\r\n",
              clk);
        DAQJMP( DAQmxStopTask( clk ));
        DAQJMP( DAQmxClearTask( clk ));
      }
      if (ao)
      {
        debug("Scanner2D: Attempting to detach DAQ AO  channel. handle: 0x%p\r\n",
              ao);
        DAQJMP( DAQmxStopTask(  ao ));
        DAQJMP( DAQmxClearTask( ao ));
      }
    Error:
      this->unlock();
      ao = NULL;
      clk = NULL;
      return status;
    }

    unsigned int
    Scanner2D::attach(void)
    {
      unsigned int status = 0; // success 0, error 1
      return_val_if(status |= this->Shutter::attach(),status);   //((Shutter*  )this)->attach(), status);
      return_val_if(status |= this->Digitizer::attach(),status); //((Digitizer*)this)->attach(), status);
      this->lock();
      Guarded_Assert(ao == NULL);
      Guarded_Assert(clk == NULL);
      DAQJMP( status = DAQmxCreateTask( "scanner2d-ao", &ao ));
      DAQJMP( status = DAQmxCreateTask( "scannner2d-clk", &clk));
      this->set_available();    
    Error:
      this->unlock();
      return status;
    }
    
    void
    _compute_galvo_waveform__constant_zero( Scanner2D::Config *cfg, float64 *data, double N )
    { memset(data,0, ((size_t)N*sizeof(float64)));
    }

    void
    _compute_linear_scan_mirror_waveform__sawtooth( LinearScanMirror::Config *cfg, float64 *data, double N )
    { int i=(int)N;
      float64 A = cfg->vpp;
      while(i--)
        data[i] = A*((i/N)-0.5); // linear ramp from -A/2 to A/2
      data[(int)N-1] = data[0];  // at end of wave, head back to the starting position
    }

    void
    _compute_pockels_vertical_blanking_waveform( Pockels::Config *cfg, float64 *data, double N )
    { int i=(int)N;
      float64 max = cfg->v_open,
              min = cfg->v_closed;
      while(i--)
        data[i] = max;           // step to max during y scan
      data[(int)N-1] = min;      // step to zero at end of scan
    }

    void
    Scanner2D::_generate_ao_waveforms(void)
    { int N = config.nsamples;
      f64 *m,*p;
      vector_f64_request(ao_workspace, 2*N - 1 /*max index*/);
      m = ao_workspace->contents; // first the mirror data
      p = m + N;                  // then the pockels data
      _compute_linear_scan_mirror_waveform__sawtooth( &((LinearScanMirror*)this)->config, m, N);
      _compute_pockels_vertical_blanking_waveform(    &(         (Pockels*)this)->config, p, N);
    }

  }

}
