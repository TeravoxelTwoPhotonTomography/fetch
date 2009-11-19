// TestAsynQ.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "../fetch/asynq.h"

#if 1
#define simple_pushpop_debug(...) debug(__VA_ARGS__)
#else
#define simple_pushpop_debug(...)
#endif

DWORD WINAPI spin(LPVOID lpParam)
{ int i = 100000;
  while(i--);
  return 0;
}

DWORD WINAPI simple_pusher(LPVOID lpParam)
{ asynq *q = (asynq*) lpParam;
  int *u,*buf = (int*) Asynq_Alloc_Token_Buffer(q);
  int n = 100;
  unsigned int res = 0;
  simple_pushpop_debug("pusher start\r\n");
  while(n--)
  { buf[0] = n;
    u = buf;
    res = Asynq_Push(q, (void**) &buf, FALSE);
    SwitchToThread();
    simple_pushpop_debug("\t+%c 0x%p %5d       %5d\r\n",
                  Asynq_Is_Full(q)?'x':' ',
                  u,
                  n,
                  q->q->ring->nelem);
  }
  Asynq_Free_Token_Buffer(buf);
  simple_pushpop_debug("pusher stop: %d\r\n",res);   
  return 0;
}

DWORD WINAPI simple_popper(LPVOID lpParam)
{ asynq *q = (asynq*) lpParam;
  int *buf = (int*) Asynq_Alloc_Token_Buffer(q);
  int n = 100;
  unsigned int res = 0;
  simple_pushpop_debug("popper start\r\n");
  while(n--)
  { res = Asynq_Pop(q,(void**)&buf);
    SwitchToThread();
    simple_pushpop_debug("\t-%c 0x%p %5d %5d\r\n",
                      Asynq_Is_Empty(q)?'x':' ',
                      res?buf:NULL,
                      n,
                      buf[0]);
  }
  Asynq_Free_Token_Buffer(buf);
  simple_pushpop_debug("popper stop: %d\r\n",res);
  return 0;
}

void simple_pushpop(size_t w, size_t h, size_t nchan, size_t bytes_per_pixel)
{ 
  asynq *q = Asynq_Alloc( 32, w*h*nchan*bytes_per_pixel );
  HANDLE threads[] = {
             CreateThread( NULL,     // use default security profile
                     32*1024*1024,   // use stack size in bytes (default 1 MB)
                     &simple_popper, // proc
                     (LPVOID) q,     // lpparam
                     CREATE_SUSPENDED,  // creation flags - set to 0 to run immediately
                     NULL),          // (output) thread id - NULL ignores
                     
             CreateThread( NULL,     // use default security profile
                     32*1024*1024,   // use stack size in bytes (default 1 MB)
                     &simple_pusher, // proc
                     (LPVOID) q,     // lpparam
                     CREATE_SUSPENDED,  // creation flags - set to 0 to run immediately                     
                     NULL)           // (output) thread id - NULL ignores
            };
  int i,nthreads = sizeof(threads)/sizeof(HANDLE);
  double x;
  TicTocTimer t;
  
  debug("          Refs: %u\r\n", q->ref_count);
  debug("Waiting pushes: %u\r\n", q->waiting_producers );
  debug("Waiting   pops: %u\r\n", q->waiting_consumers );
  debug("            &Q: 0x%p\r\n", q->q );
  Guarded_Assert( q->ref_count == 1 );
  Guarded_Assert( q->waiting_consumers == 0);
  Guarded_Assert( q->waiting_producers == 0);
  
  t = tic();  
  for(i=0;i<nthreads;i++)
  { Guarded_Assert( threads[i] );
    ResumeThread( threads[i] );
  }
  WaitForMultipleObjects( nthreads, threads, TRUE, INFINITE );
  x = toc(&t);
  debug("Elapsed: %f seconds\r\n",x);
  
  debug("          Refs: %u\r\n", q->ref_count);
  debug("Waiting pushes: %u\r\n", q->waiting_producers );
  debug("Waiting   pops: %u\r\n", q->waiting_consumers );
  
  Guarded_Assert( q->ref_count == 1 );
  Guarded_Assert( q->waiting_producers == 0 );
  Guarded_Assert( q->waiting_consumers == 0 );  
  Guarded_Assert( Asynq_Unref(q) );
}                             

int _tmain(int argc, _TCHAR* argv[])
{ Reporting_Setup_Log_To_Stdout();
  Reporting_Setup_Log_To_VSDebugger_Console();
 
  simple_pushpop( 1024,1024,8,2 );
  
	return 0;
}

