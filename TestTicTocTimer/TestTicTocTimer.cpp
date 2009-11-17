// TestTicTocTimer.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <math.h>

void minmax(void)
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
  
  debug("%zero:   %f\r\n"
        "mean :   %g us\r\n"
        "std :    %g us\r\n"
        "max :    %g us\r\n"
        "min :    %g us\r\n", 100.0*zeros/N,
                              acc  / N,
                              sqrt( acc2 / N - (acc/N)*(acc/N) ),
                              max,
                              min);                          
}

int _tmain(int argc, _TCHAR* argv[])
{ Reporting_Setup_Log_To_Stdout();
  Reporting_Setup_Log_To_VSDebugger_Console();
  minmax();
  minmax();
  minmax();
  minmax();
  minmax();
  minmax();
  minmax();
  minmax();
  minmax();
  minmax();
  return 0;
}

