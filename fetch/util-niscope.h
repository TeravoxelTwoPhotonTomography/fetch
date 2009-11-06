#pragma once
#include "niscope.h"

/* This macro handles all errors and warnings returned from NI SCOPE.
   It requires two variables be declared:
      ViStatus error
      ViChar errorSource[MAX_FUNCTION_NAME_SIZE]
   and a line marked "Error:" that marks the beginning of cleanup code.
   If a function returns an error code, "error" is set to the error code
   and "errorSource" is set to the calling function's name, and execution
   skips to the "Error:" line.  Otherwise, "error" and "errorSource" store
   the location of warnings that occur, and execution proceeds normally.    */
#undef  handleErr
#define handleErr(fCall) { int _code = (fCall);                      \
        if (_code < 0) {                                             \
           status = _code;                                          \
           strncpy( errorSource, #fCall, MAX_FUNCTION_NAME_SIZE );  \
           errorSource[MAX_FUNCTION_NAME_SIZE - 1] = '\0';          \
           strcpy( errorSource, strtok( errorSource, " (") );       \
           goto Error; }                                            \
        else if ((status == 0) && (_code > 0)) {                    \
           status = _code;                                          \
           strncpy( errorSource, #fCall, MAX_FUNCTION_NAME_SIZE );  \
           errorSource[MAX_FUNCTION_NAME_SIZE - 1] = '\0';          \
           strcpy( errorSource, strtok( errorSource, " (") ); } }

void niscope_log_wfminfo ( pf_reporter      pfOutput, 
                           niScope_wfmInfo *info );