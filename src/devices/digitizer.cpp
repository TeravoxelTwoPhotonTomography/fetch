/*
 * Digitizer.cpp
 *
 * Author: Nathan Clack <clackn@janelia.hhmi.org>
 *   Date: Apr 20, 2010
 */
/*
 * Copyright 2010 Howard Hughes Medical Institute.
 * All rights reserved.
 * Use is subject to Janelia Farm Research Campus Software Copyright 1.1
 * license terms (http://license.janelia.org/license/jfrc_copyright_1_1.html).
 */
#include "stdafx.h"
#include "../util/util-niscope.h"
#include "digitizer.h"
#include "../asynq.h"

#pragma warning(push)
#pragma warning(disable:4005)
#if 1
#define digitizer_debug(...) debug(__VA_ARGS__)
#else
#define digitizer_debug(...)
#endif
#pragma warning(pop)

#define CheckWarn( expression )  (niscope_chk( this->_vi, expression, #expression, &warning ))
#define CheckPanic( expression ) (niscope_chk( this->_vi, expression, #expression, &error   ))
#define ViErrChk( expression )    goto_if( CheckWarn(expression), Error )

namespace fetch
{
  namespace device 
  {
    NIScopeDigitizer::NIScopeDigitizer(Agent *agent)
      :IDigitizer<cfg::device::Digitizer>(agent)
      ,_vi(0)
    { __common_setup();
    }

    NIScopeDigitizer::NIScopeDigitizer(Agent *agent,Config *cfg)
      :IDigitizer<cfg::device::Digitizer>(agent,cfg)
      ,_vi(0)
    { __common_setup();
    }   

    //NIScopeDigitizer::NIScopeDigitizer(size_t nbuf, size_t nbytes_per_frame, size_t nwfm)
    //  : _vi(0)
    //{ size_t Bpp    = 2, //bytes per pixel to initially allocated for
    //         _nbuf[2] = {nbuf,nbuf},
    //           sz[2] = {nbytes_per_frame,
    //                    nwfm*sizeof(struct niScope_wfmInfo)};
    //  _alloc_qs( &this->_out, 2, _nbuf, sz );
    //}

    //Digitizer::Digitizer(const Config& cfg)
    //          : _vi(0),
    //            config(&_default_config)
    //{ config->CopyFrom(cfg);
    //  Guarded_Assert(config->IsInitialized());
    //  __common_setup();

    //} 

    NIScopeDigitizer::~NIScopeDigitizer(void)
    { if( _agent->detach() > 0 )
        warning("Could not cleanly detach. vi: %d\r\n", this->_vi);
    }

  unsigned int
    NIScopeDigitizer::detach(void)
    { ViStatus status = 1; //error
      
      digitizer_debug("Digitizer: Attempting to disarm. vi: %d\r\n", this->_vi);
      if(this->_vi)
        ViErrChk(niScope_close(this->_vi));  // Close the session
      status = 0;  // success
      digitizer_debug("Digitizer: Detached.\r\n");
Error:
      this->_vi = 0;
      return status;
    }

  unsigned int
    NIScopeDigitizer::attach(void)
    { ViStatus status = VI_SUCCESS;      
      digitizer_debug("Digitizer: Attach\r\n");      
      if( _vi == NULL )
      { // Open the NI-SCOPE instrument handle
        status = CheckPanic (
          niScope_init (const_cast<ViRsrc>(_config->name().c_str()),
                        NISCOPE_VAL_TRUE,    // ID Query
                        NISCOPE_VAL_FALSE,   // Reset?
                        &_vi)                // Session
        );
      }      
      digitizer_debug("\tGot session %3d with status %d\n",_vi,status);
      return status!=VI_SUCCESS;
    }

  void NIScopeDigitizer::__common_setup()
  {
    // Setup output queues.
    // - Sizes determine initial allocations.
    // - out[0] receives raw data     from each niScope_Fetch call.
    // - out[1] receives the metadata from each niScope_Fetch call.
    //
    size_t
      Bpp = 2, //bytes per pixel to initially allocated for
      nbuf[2] = {DIGITIZER_BUFFER_NUM_FRAMES,
                 DIGITIZER_BUFFER_NUM_FRAMES},
      sz[2] = { 
        _config->num_records() * _config->record_size() * _config->nchannels() * Bpp,
        _config->num_records() * sizeof(struct niScope_wfmInfo)};
    _alloc_qs( &_out, 2, nbuf, sz );
  }

  } // namespace fetch
} // namespace fetch