#include "stdafx.h"
#include "niScope.h"

void niscope_log_wfminfo( pf_reporter pfOutput, niScope_wfmInfo *info )
{
   (*pfOutput)
         ("--\r\n"
          "ViReal64 absoluteInitialX    %g\r\n"
          "ViReal64 relativeInitialX    %g\r\n"
          "ViReal64 xIncrement          %g\r\n"
          "ViInt32 actualSamples        %d\r\n"
          "ViReal64 offset              %g\r\n"
          "ViReal64 gain                %g\r\n"
          "ViReal64 reserved1           %g\r\n"
          "ViReal64 reserved2           %g\r\n"
          "--\r\n"
          , info->absoluteInitialX
          , info->relativeInitialX
          , info->xIncrement
          , info->actualSamples
          , info->offset
          , info->gain
          , info->reserved1
          , info->reserved2 );
}
