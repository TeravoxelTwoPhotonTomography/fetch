#include "stdafx.h"
#include "util-niscope.h"
#include "microscope.h"
#include "device-digitizer.h"

Digitizer            g_digitizer        = {0};
//Digitizer_Config     g_digitizer_config = DIGITIZER_CONFIG_DEFAULT;
Digitizer_Config     g_digitizer_config =   {"Dev6\0",
                       DIGITIZER_MAX_SAMPLE_RATE,
                       1024,
                       0.0,
                       {{"0\0",2.0,NISCOPE_VAL_DC,VI_TRUE},
                        {"1\0",2.0,NISCOPE_VAL_DC,VI_TRUE},
                        {"2\0",2.0,NISCOPE_VAL_DC,VI_TRUE},
                        {"3\0",2.0,NISCOPE_VAL_DC,VI_TRUE},
                        {"4\0",2.0,NISCOPE_VAL_DC,VI_FALSE},
                        {"5\0",2.0,NISCOPE_VAL_DC,VI_FALSE},
                        {"6\0",2.0,NISCOPE_VAL_DC,VI_FALSE},
                        {"7\0",2.0,NISCOPE_VAL_DC,VI_FALSE}}
                       };

#define NISCOPE_DEFAULT_ERROR_HANDLER_DECLARATIONS \
  ViStatus status = VI_SUCCESS;\
  ViChar   errorSource[MAX_FUNCTION_NAME_SIZE];\
  ViChar   errorMessage[MAX_ERROR_DESCRIPTION] = " "  

#define NISCOPE_DEFAULT_ERROR_HANDLER(msg) \
  if (status != VI_SUCCESS)\
  { if( g_digitizer.vi )\
    { niScope_errorHandler (g_digitizer.vi, status, errorSource, errorMessage);\
      error( "%s\r\n%s\r\n", (msg), errorMessage );\
    } else\
    { error( "%s\r\n\tDigitizer ViSession not initialized.\r\n");\
    }\
  }

void Digitizer_Init(void)
{ // Register Shutdown function
  Register_New_Shutdown_Callback( &Digitizer_Close );
  Register_New_Microscope_Hold_Callback( &Digitizer_Hold );
  Register_New_Microscope_Off_Callback( &Digitizer_Off );
}

unsigned int Digitizer_Close(void)
{ NISCOPE_DEFAULT_ERROR_HANDLER_DECLARATIONS;
  debug("Digitizer: Close.\r\n");
  ViSession vi = g_digitizer.vi;
  // Close the session
  if (vi)
    handleErr( niScope_close (vi) );  
Error:
  g_digitizer.vi = NULL;
  NISCOPE_DEFAULT_ERROR_HANDLER("Digitizer_Close");
  return status;
}

unsigned int Digitizer_Off(void)
{ debug("Digitizer: Off\r\n");
  return Digitizer_Close();
}

unsigned int Digitizer_Hold(void)
{ NISCOPE_DEFAULT_ERROR_HANDLER_DECLARATIONS;
  ViSession *vi = &g_digitizer.vi;

  debug("Digitizer: Hold\r\n");
  if( (*vi) == NULL )
  { // Open the NI-SCOPE instrument handle
    handleErr (
      niScope_init (g_digitizer_config.resource_name, 
                    NISCOPE_VAL_TRUE,  // ID Query
                    NISCOPE_VAL_FALSE, // Reset?
                    vi)                // Session
    );
  }
Error:
  NISCOPE_DEFAULT_ERROR_HANDLER("Digitizer_Hold");
  return status;
}