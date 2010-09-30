#include "stdafx.h"
#include "util-nidaqmx.h"

#define UTIL_NIDAQ_ERROR_BUFFER_SIZE 2048

int32 Guarded_DAQmx( int32 error, const char* expression, pf_reporter report )
{	char  errBuff[UTIL_NIDAQ_ERROR_BUFFER_SIZE]={'\0'},      
        errBuffEx[UTIL_NIDAQ_ERROR_BUFFER_SIZE]={'\0'};
  return_val_if( error == DAQmxSuccess, error );                      // check fail
  DAQmxGetErrorString(error, errBuff ,UTIL_NIDAQ_ERROR_BUFFER_SIZE);  // get error message
  DAQmxGetExtendedErrorInfo(errBuffEx,UTIL_NIDAQ_ERROR_BUFFER_SIZE);  // get error message
  if( DAQmxFailed(error) )
    (*report)( "%s\r\n%s\r\n%s\r\n", (expression), errBuff, errBuffEx );// report
  else
    warning( "%s\r\n%s\r\n%s\r\n", (expression), errBuff, errBuffEx );  // report a warning
  return error;
}