// TestAsynQ.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "../fetch/asynq.h"

DWORD WINAPI spin(LPVOID lpParam)
{ int i = 100000;
  while(i--);
  return 0;
}

DWORD WINAPI simple_pusher(LPVOID lpParam)
{ asynq *q = (asynq*) lpParam;
  void *buf = Asynq_Alloc_Token_Buffer(q);
  int n = 100;
  unsigned int res = 0;
  debug("pusher start\r\n");
  while(n--)
  { res |= Asynq_Push(q,&buf,TRUE);
    debug("\t+ %p %5d\r\n", &buf, n);
  }
  Asynq_Free_Token_Buffer(buf);
  debug("pusher stop: %d\r\n",res);
  return 0;
}

DWORD WINAPI simple_popper(LPVOID lpParam)
{ asynq *q = (asynq*) lpParam;
  void *buf = Asynq_Alloc_Token_Buffer(q);
  int n = 100;
  unsigned int res = 0;
  debug("popper start\r\n");
  while(1)
  { if( Asynq_Pop(q,&buf) )
      debug("\t- %p %5d\r\n", &buf, n);
  }
  Asynq_Free_Token_Buffer(buf);
  debug("popper stop: %d\r\n",res);
  return 0;
}

void simple_pushpop(size_t w, size_t h, size_t nchan, size_t bytes_per_pixel)
{ 
  asynq *q = Asynq_Alloc( 2, w*h*nchan*bytes_per_pixel );
  HANDLE threads[] = {
             CreateThread( NULL,     // use default security profile
                     32*1024*1024,   // use stack size in bytes (default 1 MB)
                     &simple_pusher, // proc
                     (LPVOID) q,     // lpparam
                     CREATE_SUSPENDED,  // creation flags - set to 0 to run immediately
                     NULL),          // (output) thread id - NULL ignores
                     
             CreateThread( NULL,     // use default security profile
                     32*1024*1024,   // use stack size in bytes (default 1 MB)
                     &simple_popper, // proc
                     (LPVOID) q,     // lpparam
                     CREATE_SUSPENDED,  // creation flags - set to 0 to run immediately                     
                     NULL)           // (output) thread id - NULL ignores
            };
  int i,nthreads = sizeof(threads)/sizeof(HANDLE);
  double x;
  TicTocTimer t = tic();
  
  for(i=0;i<nthreads;i++)
  { Guarded_Assert( threads[i] );
    ResumeThread( threads[i] );
  }
  WaitForMultipleObjects( nthreads, threads, TRUE, INFINITE );
  x = toc(&t);
  debug("Elapsed: %f seconds\r\n",x);


}                             

int _tmain(int argc, _TCHAR* argv[])
{ Reporting_Setup_Log_To_Stdout();
  Reporting_Setup_Log_To_VSDebugger_Console();
 
  simple_pushpop( 1024,1024,8,2 );
  
	return 0;
}

