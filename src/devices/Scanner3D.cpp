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
#include <google\protobuf\descriptor.h>
#include "Scanner3D.h"
#include "Scanner2D.h"


#define DAQWRN( expr )        (Guarded_DAQmx( (expr), #expr, warning))
#define DAQERR( expr )        (Guarded_DAQmx( (expr), #expr, error  ))
#define DAQJMP( expr )        goto_if_fail( 0==DAQWRN(expr), Error)

namespace fetch
{ namespace device
  {
    void set_unset_fields(google::protobuf::Message *msg)
    { const google::protobuf::Descriptor *d = msg->GetDescriptor();
      const google::protobuf::Reflection *r = msg->GetReflection();
      for(int i=0; i<d->field_count();++i)
      { const google::protobuf::FieldDescriptor *field = d->field(i);
      google::protobuf::Message *child = NULL;
        switch(field->type())
        {
          case google::protobuf::FieldDescriptor::TYPE_MESSAGE:
            if(field->is_repeated())
            { for(int j=0; j<r->FieldSize(*msg,field);++j)
              {
                child = r->MutableRepeatedMessage(msg,field,j);
                set_unset_fields(child);
              }

            } else
            {
              child = r->MutableMessage(msg,field,NULL);           
              set_unset_fields(child);
            }
            break;
          default:
            if(field->has_default_value() && !r->HasField(*msg,field))
            { switch(field->cpp_type())
              {
                case google::protobuf::FieldDescriptor::CPPTYPE_BOOL:  r->SetBool(msg,field,field->default_value_bool()); break;
                case google::protobuf::FieldDescriptor::CPPTYPE_ENUM:  r->SetEnum(msg,field,field->default_value_enum()); break;
                case google::protobuf::FieldDescriptor::CPPTYPE_DOUBLE:r->SetDouble(msg,field,field->default_value_double()); break;
                case google::protobuf::FieldDescriptor::CPPTYPE_FLOAT: r->SetFloat(msg,field,field->default_value_float()); break;
                case google::protobuf::FieldDescriptor::CPPTYPE_INT32: r->SetInt32(msg,field,field->default_value_int32()); break;                 
                case google::protobuf::FieldDescriptor::CPPTYPE_INT64: r->SetInt64(msg,field,field->default_value_int64()); break;                  
                case google::protobuf::FieldDescriptor::CPPTYPE_UINT32:r->SetUInt32(msg,field,field->default_value_uint32()); break;                  
                case google::protobuf::FieldDescriptor::CPPTYPE_UINT64:r->SetUInt64(msg,field,field->default_value_uint64()); break;                  
                case google::protobuf::FieldDescriptor::CPPTYPE_STRING:r->SetString(msg,field,field->default_value_string()); break;
                default:
                  warning("Got unexpected CppType\n");
              }
            }
        }        
      }
    }

    Scanner3D::Scanner3D(void)
      : config(Configurable<cfg::device::Scanner3D>::_config)
    { 
      debug("here\n");
      set_config(config);
      this->NIScopeDigitizer::config->add_channel()->set_enabled(true);
      this->NIScopeDigitizer::config->add_channel()->set_enabled(true);
      set_unset_fields(config);
      debug("%s\n",config->DebugString().c_str());
    }

    Scanner3D::Scanner3D( Config *cfg )
    : Configurable<cfg::device::Scanner3D>::Configurable(cfg),
      ZPiezo(cfg->mutable_zpiezo()),
      Scanner2D(cfg->mutable_scanner2d()),
      config(Configurable<cfg::device::Scanner3D>::_config)
    {}

    Scanner3D::~Scanner3D(void)
    { if (this->detach()>0)
          warning("Could not cleanly detach Scanner3D. (addr = 0x%p)\r\n",this);
    }
    
    unsigned int
    Scanner3D::
    attach(void) 
    { return this->Scanner2D::attach(); // Returns 0 on success, 1 otherwise
    }
    
    unsigned int
    Scanner3D::
    detach(void)
    { return this->Scanner2D::detach(); // Returns 0 on success, 1 otherwise
    }
    
    void
    Scanner3D::
    _generate_ao_waveforms(void)
    { this->_generate_ao_waveforms(0.0);
    }
    
    void
    Scanner3D::
    _generate_ao_waveforms(f64 z_um)
    { 
      int N = this->Scanner2D::config->ao_samples_per_frame();
      f64 *m,*p, *z;
      lock();      
      vector_f64_request(ao_workspace, 3*N - 1 /*max index*/);
      m = ao_workspace->contents; // first the mirror data
      z = m + N;                  // then the zpiezo  data
      p = z + N;                  // then the pockels data
      _compute_linear_scan_mirror_waveform__sawtooth( this->NIDAQLinearScanMirror::config, m, N);
      _compute_zpiezo_waveform_const(                 this->ZPiezo::config    , z_um, z, N);
      _compute_pockels_vertical_blanking_waveform(    this->NIDAQPockels::config         , p, N);
#if 0
      vector_f64_dump(ao_workspace,"out.f64");
#endif
      unlock();
    }

    void
    Scanner3D::
    _generate_ao_waveforms__z_ramp_step(f64 z_um)
    {
      int N = this->Scanner2D::config->ao_samples_per_frame();
      f64 *m,*p, *z;
      lock();
      vector_f64_request(ao_workspace, 3*N - 1 /*max index*/);
      m = ao_workspace->contents; // first the mirror data
      z = m + N;                  // then the zpiezo  data
      p = z + N;                  // then the pockels data
      _compute_linear_scan_mirror_waveform__sawtooth( this->NIDAQLinearScanMirror::config, m, N);
      _compute_zpiezo_waveform_ramp(                  this->ZPiezo::config    , z_um, z, N);
      _compute_pockels_vertical_blanking_waveform(    this->NIDAQPockels::config         , p, N);
#if 0
      vector_f64_dump(ao_workspace,"out.f64");
#endif
      unlock();
    }

    #define MAX_CHAN_STRING 1024
    static void
    _setup_ao_chan(TaskHandle cur_task,
                   double     freq,
                   device::Scanner2D::Config        *cfg,
                   device::NIDAQLinearScanMirror::Config *lsm_cfg,
                   device::NIDAQPockels::Config          *pock_cfg,
                   device::ZPiezo::Config           *zpiezo_cfg)
    {
      char aochan[MAX_CHAN_STRING];
      f64 vmin,vmax;

      // concatenate the channel names
      assert( POCKELS_MAX_CHAN_STRING
             +LINEAR_SCAN_MIRROR__MAX_CHAN_STRING
             +zpiezo_cfg->channel().size()
             +2 < MAX_CHAN_STRING);
      memset(aochan, 0, sizeof(aochan));
      strcat(aochan, lsm_cfg->ao_channel().c_str());
      strcat(aochan, ",");
      strcat(aochan, zpiezo_cfg->channel().c_str());
      strcat(aochan, ",");
      strcat(aochan, pock_cfg->ao_channel().c_str());

      vmin = MIN( lsm_cfg->v_lim_min(), pock_cfg->v_lim_min() );
      vmin = MIN( vmin, zpiezo_cfg->v_lim_min() );
      vmax = MAX( lsm_cfg->v_lim_max(), pock_cfg->v_lim_max() );
      vmax = MAX( vmax, zpiezo_cfg->v_lim_max() );
      { f64 v[4];
        // NI DAQ's typically have multiple voltage ranges capable of achieving different precisions.
        // The 6259 has 2 ranges.
        DAQERR(DAQmxGetDevAOVoltageRngs("Dev1",v,4));        // FIXME: HACK - need to get device name
        vmin = MAX(vmin,v[2]);        
        vmax = MIN(vmax,v[3]);        
      }            

      DAQERR( DAQmxCreateAOVoltageChan(cur_task,
              aochan,                                  //eg: "/Dev1/ao0,/Dev1/ao2,/Dev1/ao1"
              "vert-mirror-out,zpiezo-out,pockels-out",//name to assign to channel
              vmin,                                    //Volts eg: -10.0
              vmax,                                    //Volts eg:  10.0
              DAQmx_Val_Volts,                         //Units
              NULL));                                  //Custom scale (none)

      DAQERR( DAQmxCfgAnlgEdgeStartTrig(cur_task,
              cfg->trigger().c_str(),
              DAQmx_Val_Rising,
              0.0));

      DAQERR( DAQmxCfgSampClkTiming(cur_task,
              cfg->clock().c_str(),          // "Ctr1InternalOutput",
              freq,
              DAQmx_Val_Rising,
              DAQmx_Val_ContSamps, // use continuous output so that counter stays in control
              cfg->ao_samples_per_frame()));
    }

    void
    Scanner3D::_config_daq()
    { TaskHandle             cur_task = 0;

      ViInt32   N          = Scanner2D::config->ao_samples_per_frame();
      float64   frame_time = Scanner2D::config->nscans() / Scanner2D::config->frequency_hz();  //  512 records / (7920 records/sec)
      float64   freq       = N/frame_time;                         // 4096 samples / 64 ms = 63 kS/s

      // set up counter for sample clock
      // - A finite pulse sequence is generated by a pair of onboard counters.
      //   In testing, it appears that after the device is reset, initializing
      //   the counter task doesn't work quite right.  First, I have to start the
      //   task with the paired counter once.  Then, I can set things up normally.
      //   After initializing with the paired counter once, things work fine until
      //   the device (or computer) is reset.  My guess is this is a fault of the
      //   board or driver software.
      // - below, we just cycle the counters when config gets called.  This ensures
      //   everything configures correctly the first time, even after a device
      //   reset or cold start.
      // The "fake" initialization
      DAQERR( DAQmxClearTask(this->clk) );                  // Once a DAQ task is started, it needs to be cleared before restarting
      DAQERR( DAQmxCreateTask("scanner3d-clk",&this->clk)); //
      cur_task = this->clk;
      DAQERR( DAQmxCreateCOPulseChanFreq       ( cur_task,
                                                 this->Scanner2D::config->ctr_alt().c_str(),     // "Dev1/ctr0"
                                                 "sample-clock",
                                                 DAQmx_Val_Hz,
                                                 DAQmx_Val_Low,
                                                 0.0,
                                                 freq,
                                                 0.5 ));
      DAQERR( DAQmxStartTask(cur_task) );

      // The "real" initialization
      DAQERR( DAQmxClearTask(this->clk) );                  // Once a DAQ task is started, it needs to be cleared before restarting
      DAQERR( DAQmxCreateTask("scanner3d-clk",&this->clk)); //
      cur_task = this->clk;
      DAQERR( DAQmxCreateCOPulseChanFreq       ( cur_task,
                                                 this->Scanner2D::config->ctr().c_str(),     // "Dev1/ctr1"
                                                 "sample-clock",
                                                 DAQmx_Val_Hz,
                                                 DAQmx_Val_Low,
                                                 0.0,
                                                 freq,
                                                 0.5 ));

      DAQERR( DAQmxCfgImplicitTiming           ( cur_task, DAQmx_Val_FiniteSamps, N ));
      DAQERR( DAQmxCfgDigEdgeStartTrig         ( cur_task, "AnalogComparisonEvent", DAQmx_Val_Rising ));
      DAQERR( DAQmxSetArmStartTrigType         ( cur_task, DAQmx_Val_DigEdge ));
      DAQERR( DAQmxSetDigEdgeArmStartTrigSrc   ( cur_task, this->Scanner2D::config->armstart().c_str() ));
      DAQERR( DAQmxSetDigEdgeArmStartTrigEdge  ( cur_task, DAQmx_Val_Rising ));
#if 0
      { f64 rate;
        DAQWRN( DAQmxGetCOCtrTimebaseRate(cur_task,this->Scanner2D::config->ctr().c_str(),&rate));
        debug("Ctr Timebase: %f\r\n",rate);        
      }
#endif
      
      //
      // VERTICAL
      //

      // set up ao task - vertical
      cur_task = this->ao;

      // Setup AO channels
      DAQERR( DAQmxClearTask(this->ao) );                   // Once a DAQ task is started, it needs to be cleared before restarting
      DAQERR( DAQmxCreateTask( "scanner3d-ao", &this->ao)); //
      _setup_ao_chan(this->ao,
                     freq,
                     this->Scanner2D::config,
                     this->NIDAQLinearScanMirror::config,
                     this->NIDAQPockels::config,
                     this->ZPiezo::config);
      DAQERR( DAQmxSetWriteRegenMode(this->ao,DAQmx_Val_DoNotAllowRegen));
      this->_generate_ao_waveforms();
      this->_register_daq_event();
      
      // Set up the shutter control
      this->Shutter::Bind();

      return;
    }

    /*
     * Compute ZPiezo Waveform Ramp:
     * ----------------------------
     *
     *           0    N
     *           |    |____________  ,- z_um + z_step
     *               /
     *              /
     *             /
     *            /
     *  _________/                   ,- z_um
     *
     *  Notice that N-1 is equiv. to z_step - z_step/N
     */
    void
    Scanner3D::_compute_zpiezo_waveform_ramp( ZPiezo::Config *cfg, f64 z_um, f64 *data, f64 N )
    { int i=(int)N;
      f64 A = cfg->um_step() * cfg->um2v(),
              off = z_um * cfg->um2v();
      while(i--)
        data[i] = A*(i/(N-1))+off; // linear ramp from off to off+A
    }

    /*
     * Compute ZPiezo Waveform Constant:
     * ----------------------------
     *         0     N
     *         |     |
     *          ____________________ ,- z_um
     *  _______|                     ,- z previous
     */
    void
    Scanner3D::_compute_zpiezo_waveform_const( ZPiezo::Config *cfg, f64 z_um, f64 *data, f64 N )
    { int i=(int)N;
      f64 off = z_um * cfg->um2v();
      while(i--)
        data[i] = off; // linear ramp from off to off+A
    }

    void Scanner3D::set_config(Config *cfg)
    { config = cfg;
      debug("Scanner3D: setting config to 0x%p\n",cfg);
      this->Scanner2D::set_config(cfg->mutable_scanner2d());
      this->ZPiezo::set_config(cfg->mutable_zpiezo());
    }

  }
}
