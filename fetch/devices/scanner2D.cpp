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
#include "frame.h"

#define DAQWRN( expr )        (Guarded_DAQmx( (expr), #expr, warning))
#define DAQERR( expr )        (Guarded_DAQmx( (expr), #expr, error  ))
#define DAQJMP( expr )        goto_if_fail( 0==DAQWRN(expr), Error)

namespace fetch
{

  namespace device
  {

    Scanner2D::Scanner2D() :
      config((Scanner2D::Config)SCANNER2D_DEFAULT_CONFIG), ao(NULL), clk(NULL)
    {
    }

    Scanner2D::~Scanner2D()
    {
      // TODO Auto-generated destructor stub
    }

    ViInt32
    Scanner2D::_compute_record_size(void)
    {
      f32 duty = this->config.line_duty_cycle, rate =
          ((Digitizer*) this)->config.sample_rate, freq =
          this->config.frequency_Hz;
      return (ViInt32)(duty * rate / freq);
    }

    Frame_With_Interleaved_Lines
    Scanner2D::_describe_frame()
    {
      ViInt32 samples_per_scan = _compute_record_size();
      Frame_With_Interleaved_Lines format((u16) samples_per_scan,
          config.nscans, ((Digitizer*) this)->config.num_channels, id_i16);
      Guarded_Assert( format.nchan > 0 );
      Guarded_Assert( format.height > 0 );
      Guarded_Assert( format.width > 0 );
      return format;
    }

    unsigned int
    Scanner2D::detach(void)
    { unsigned int status = 0; // success 0, error 1

      if( !this->disarm(SCANNER2D_DEFAULT_TIMEOUT) )
        warning("Could not cleanly disarm Scanner2D.\r\n");

      status |= ((Shutter*)   (this))->detach();
      status |= ((Digitizer*) (this))->detach();

      this->lock();
      if( clk )
      { debug("Scanner2D: Attempting to detach DAQ CLK channel. handle: 0x%p\r\n", clk);
        DAQJMP( DAQmxStopTask(  clk ));
        DAQJMP( DAQmxClearTask( clk ));
      }
      if( ao )
      { debug("Scanner2D: Attempting to detach DAQ AO  channel. handle: 0x%p\r\n", ao );
        DAQJMP( DAQmxStopTask(  daq_ao ));
        DAQJMP( DAQmxClearTask( daq_ao ));
      }
    Error:
      this->unlock();
      ao  = NULL;
      clk = NULL;
      return status;
    }

    unsigned int
    Scanner2D::attach(void)
    { unsigned int status = 0; // success 0, error 1
      return_val_if( status |= ((Shutter*) (this))->attach()   ,status);
      return_val_if( status |= ((Digitizer*) (this))->attach() ,status);

      this->lock();
      Guarded_Assert( ao == NULL);
      Guarded_Assert(clk == NULL);
      status = DAQERR( DAQmxCreateTask( "scanner2d-ao", &ao ));
      status = DAQERR( DAQmxCreateTask( "scannner2d-clk", &clk));
      Device_Set_Available( gp_scanner_device );
    Error:
      this->unlock();
      return status;
    }

  }

}
