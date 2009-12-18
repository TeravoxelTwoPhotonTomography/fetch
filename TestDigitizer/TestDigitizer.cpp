// TestDigitizer.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <conio.h>

#include "../fetch/device.h"
#include "../fetch/device-digitizer.h"

int _tmain(int argc, _TCHAR* argv[])
{ Device *digitizer = NULL;

  Reporting_Setup_Log_To_VSDebugger_Console();
  Reporting_Setup_Log_To_Stdout();

  Digitizer_Init();
  digitizer = Digitizer_Get_Device();
  Digitizer_Attach();

  Device_Arm( digitizer, 
              Digitizer_Get_Default_Task(),
              INFINITE );
#if 1              
  Device_Run( digitizer );
  while(  !_kbhit()  ) Sleep(1);
#else
  // Exec device config function
  if( !(digitizer->task->cfg_proc)(digitizer, digitizer->task->in, digitizer->task->out) )
    error("While configuring, something went wrong with the device configuration.\r\n");
    if( !(digitizer->task->run_proc)(digitizer, digitizer->task->in, digitizer->task->out) )
    error("While running, something went wrong with the device configuration.\r\n");  
#endif
  
  Device_Stop  ( digitizer, INFINITE );
  Device_Disarm( digitizer, INFINITE );
  
  Digitizer_Detach();
  Digitizer_Destroy();
    
  //while(  !_kbhit()  ) Sleep(1);
	return 0;
}

