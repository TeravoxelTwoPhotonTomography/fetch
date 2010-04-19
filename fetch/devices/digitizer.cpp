#include "stdafx.h"
#include "util/util-niscope.h"
#include "devices/digitizer.h"
#include "asynq.h"

#if 1
#define digitizer_debug(...) debug(__VA_ARGS__)
#else
#define digitizer_debug(...)
#endif

#define CheckWarn( expression )  (niscope_chk( g_digitizer.vi, expression, #expression, &warning ))
#define CheckPanic( expression ) (niscope_chk( g_digitizer.vi, expression, #expression, &error   ))
#define ViErrChk( expression )    goto_if( CheckWarn(expression), Error )

namespace fetch
{
    Digitizer::Digitizer(void)
      : config(DIGITIZER_CONFIG_DEFAULT)
    { 
      // Setup output queues.
      // - Sizes determine initial allocations.
      // - out[0] recieves raw data     from each niScope_Fetch call.
      // - out[1] recieves the metadata from each niScope_Fetch call.
      //
      size_t Bpp    = 2; //bytes per pixel to initially allocated for
             nbuf[2] = {DIGITIZER_BUFFER_NUM_FRAMES,
                        DIGITIZER_BUFFER_NUM_FRAMES},
               sz[2] = { config->num_records * config->record_length * config->num_channels * Bpp;
                         config->num_records * sizeof(struct niScope_wfmInfo)};
      _alloc_qs( &this->out, 2, nbuf, sz );
    }

  unsigned int
    Digitizer::detach(void)
    { ViStatus status = 1; //error
      
      digitizer_debug("Digitizer: Attempting to detach. vi: %d\r\n", this->vi);
      if( !this->disarm( DIGITIZER_DEFAULT_TIMEOUT ) )
        warning("Could not cleanly disarm digitizer.\r\n");

      this->lock();
      if(this->vi)
        ViErrChk(niScope_close(this->vi));  // Close the session

      status = 0;  // success
      digitizer_debug("Digitizer: Detached.\r\n");
    Error:  
      this->vi = 0;
      this->_is_available = 0;  
      this->unlock();
      return status;
    }

  unsigned int
    Digitizer::attach(void)
    { ViStatus status = VI_SUCCESS;
      this->lock();
      digitizer_debug("Digitizer: Attach\r\n");
      { ViSession *vi = &this->vi;

        if( (*vi) == NULL )
        { // Open the NI-SCOPE instrument handle
          status = CheckPanic (
            niScope_init (this->config.resource_name, 
                          NISCOPE_VAL_TRUE,  // ID Query
                          NISCOPE_VAL_FALSE, // Reset?
                          vi)                // Session
          );
        }
      }
      this->set_available();
      digitizer_debug("\tGot session %3d with status %d\n",this->vi,status);
      this->unlock();

      return status;
    }

} // namespace fetch
