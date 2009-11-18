// TestTicTocTimer.cpp : Defines the entry point for the console application.
//


#include "stdafx.h"
#include <math.h>

void latency(void)
{ int iter = 1000000;
  TicTocTimer t  = tic(),
             *pt = &t;
  double  x, 
          acc   =  0.0, 
          acc2  =  0.0, 
          max   = -DBL_MAX,
          min   =  DBL_MAX,
          N     =  iter;  
  unsigned zeros = 0;
  
  while(iter--)
  { toc(&t);
    x = toc(&t)*1e6; // convert to microseconds
    acc  += x;
    acc2 += x*x;
    max   = MAX(max,x);
    min   = MIN(min,x);
    zeros += (x<1e-6); // count differences less than a picosecond
  }
  
  debug("zeros:   %f %% \r\n"
        "mean :   %g us\r\n"
        "std :    %g us\r\n"
        "max :    %g us\r\n"
        "min :    %g us\r\n", 100.0*zeros/N,
                              acc  / N,
                              sqrt( acc2 / N - (acc/N)*(acc/N) ),
                              max,
                              min);                          
}

void slp(void)
{ int iter = 30;
  TicTocTimer t  = tic(),
             *pt = &t;
  double  x, 
          acc   =  0.0, 
          acc2  =  0.0, 
          max   = -DBL_MAX,
          min   =  DBL_MAX,
          N     =  iter;  
  unsigned zeros = 0;
  
  while(iter--)
  { toc(&t);
    Sleep(100.0);
    x = toc(&t)*1e3; // convert to milliseconds
   
    acc  += x;
    acc2 += x*x;
    max   = MAX(max,x);
    min   = MIN(min,x);
    zeros += (x<1e-6); // count differences less than a picosecond
  }
  
  debug("zeros:   %f percent\r\n"
        "mean :   %g ms\r\n"
        "std :    %g ms\r\n"
        "max :    %g ms\r\n"
        "min :    %g ms\r\n", 100.0*zeros/N,
                              acc  / N,
                              sqrt( acc2 / N - (acc/N)*(acc/N) ),
                              max,
                              min);                          
}

int _tmain(int argc, _TCHAR* argv[])
{ 
  Reporting_Setup_Log_To_Stdout();
  Reporting_Setup_Log_To_VSDebugger_Console();
  latency();
  slp();
  return 0;
}

