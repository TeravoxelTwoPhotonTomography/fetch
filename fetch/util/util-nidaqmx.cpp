#include "stdafx.h"
#include "util-nidaqmx.h"

#define UTIL_NIDAQ_ERROR_BUFFER_SIZE 2048

int32 Guarded_DAQmx( int32 error, const char* expression, pf_reporter report )
{	char        errBuff[UTIL_NIDAQ_ERROR_BUFFER_SIZE]={'\0'};
  return_val_if_fail( DAQmxFailed(error), error );                  // check fail
  DAQmxGetExtendedErrorInfo(errBuff,UTIL_NIDAQ_ERROR_BUFFER_SIZE);  // get error message
  (*report)( "%s\r\n%s\r\n", (expression), errBuff );               // report
  return error;
}