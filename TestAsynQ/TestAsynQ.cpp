// TestAsynQ.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "../fetch/asynq.h"

#if 1
#define testasynq_debug(...) debug(__VA_ARGS__)
#else
#define testasynq_debug(...)
#endif

void spin(int i)
{ while(i--);
}

//
// Pushpop1
// --------
//  - push with expansion
//  - some load on producer thread (producer bottlenecked)
//  - one producer/one consumer (pop)

DWORD WINAPI pushpop1_pusher(LPVOID lpParam)
{ asynq *q = Asynq_Ref( (asynq*) lpParam );
  u8 *u,*buf = (u8*) Asynq_Token_Buffer_Alloc(q);
  int n = 100;
  unsigned int res = 1;
  testasynq_debug("pusher start\r\n");
  while(n--)
  { memset(buf,n,q->q->buffer_size_bytes);
    u = buf;
    res &= Asynq_Push(q, (void**) &buf, TRUE);    
    testasynq_debug("\t+%c 0x%p %5d       %5d\r\n",
                  Asynq_Is_Full(q)?'x':' ',
                  u,
                  n,
                  q->q->ring->nelem);
  }
  Asynq_Token_Buffer_Free(buf);
  testasynq_debug("pusher stop: %d\r\n",res);
  Asynq_Unref(q);   
  ExitThread(res==0);
  return 0;
}

DWORD WINAPI pushpop1_popper(LPVOID lpParam)
{ asynq *q = Asynq_Ref( (asynq*) lpParam );
  u8 *buf = (u8*) Asynq_Token_Buffer_Alloc(q);
  int n = 100;
  unsigned int res = 1;
  testasynq_debug("popper start\r\n");
  while(n--)
  { res &= Asynq_Pop(q,(void**)&buf);    
    testasynq_debug("\t-%c 0x%p %5d %5u\r\n",
                      Asynq_Is_Empty(q)?'x':' ',
                      res?buf:NULL,
                      n,
                      buf[0]);
  }
  Asynq_Token_Buffer_Free(buf);
  testasynq_debug("popper stop: %d\r\n",res);
  Asynq_Unref(q);
  ExitThread(res==0);
  return 0;
}

void pushpop1(size_t w, size_t h, size_t nchan, size_t bytes_per_pixel)
{ 
  asynq *q = Asynq_Alloc( 4, w*h*nchan*bytes_per_pixel );
  HANDLE threads[] = {
             CreateThread( NULL,     // use default security profile
                     1024*1024,      // use stack size in bytes (default 1 MB)
                     &pushpop1_popper, // proc
                     (LPVOID) q,     // lpparam
                     CREATE_SUSPENDED,  // creation flags - set to 0 to run immediately
                     NULL),          // (output) thread id - NULL ignores
                     
             CreateThread( NULL,     // use default security profile
                     1024*1024,      // use stack size in bytes (default 1 MB)
                     &pushpop1_pusher, // proc
                     (LPVOID) q,     // lpparam
                     CREATE_SUSPENDED,  // creation flags - set to 0 to run immediately                     
                     NULL)           // (output) thread id - NULL ignores
            };
  int i,nthreads = sizeof(threads)/sizeof(HANDLE);
  double x;
  TicTocTimer t;
  
  debug("Before\r\n");
  debug("\t          Refs: %u\r\n", q->ref_count);
  debug("\tWaiting pushes: %u\r\n", q->waiting_producers );
  debug("\tWaiting   pops: %u\r\n", q->waiting_consumers );
  debug("\t            &Q: 0x%p\r\n", q->q );
  debug("\t    Q capacity: %d\r\n", q->q->ring->nelem );
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
  debug("After:\r\n");
  debug("\tElapsed: %f seconds\r\n",x);
  debug("\t          Refs: %u\r\n", q->ref_count);
  debug("\tWaiting pushes: %u\r\n", q->waiting_producers );
  debug("\tWaiting   pops: %u\r\n", q->waiting_consumers );
  debug("\t    Q capacity: %d\r\n", q->q->ring->nelem );
  Guarded_Assert( q->ref_count == 1 );
  Guarded_Assert( q->waiting_producers == 0 );
  Guarded_Assert( q->waiting_consumers == 0 );  
  Guarded_Assert( Asynq_Unref(q) );
  
  while( nthreads-- )
    Guarded_Assert_WinErr( CloseHandle( threads[nthreads] ));  
}                             

//
// Pushpop2
// --------
//  - push with no expansion
//  - no load
//  - one producer/one consumer

DWORD WINAPI pushpop2_pusher(LPVOID lpParam)
{ asynq *q = Asynq_Ref( (asynq*) lpParam );
  u8 *buf = (u8*) Asynq_Token_Buffer_Alloc(q);
  int n = 100;
  unsigned int res = 1;
  testasynq_debug("pusher start\r\n");
  while(n--)  
    res &= Asynq_Push(q, (void**) &buf, FALSE);
  Asynq_Token_Buffer_Free(buf);
  testasynq_debug("pusher stop: %d\r\n",res);
  Asynq_Unref(q);   
  ExitThread(res==0); //return 1 on fail
  return 0;
}

DWORD WINAPI pushpop2_popper(LPVOID lpParam)
{ asynq *q = Asynq_Ref( (asynq*) lpParam );
  u8 *buf = (u8*) Asynq_Token_Buffer_Alloc(q);
  int n = 100;
  unsigned int res = 1;
  testasynq_debug("popper start\r\n");
  while(Asynq_Pop_Timed(q,(void**)&buf,500)) 
    n--;    
  Asynq_Token_Buffer_Free(buf);
  testasynq_debug("An abandoned wait was expected.\r\n",res);
  testasynq_debug("popper stop: %d\r\n",res);
  Asynq_Unref(q);
  ExitThread(0); // return 1 on fail
  return 0;
}

void pushpop2(size_t w, size_t h, size_t nchan, size_t bytes_per_pixel)
{ 
  asynq *q = Asynq_Alloc(1, w*h*nchan*bytes_per_pixel );
  HANDLE threads[] = {
             CreateThread( NULL,     // use default security profile
                     1024*1024,      // use stack size in bytes (default 1 MB)
                     &pushpop2_popper, // proc
                     (LPVOID) q,     // lpparam
                     CREATE_SUSPENDED,  // creation flags - set to 0 to run immediately
                     NULL),          // (output) thread id - NULL ignores
                     
             CreateThread( NULL,     // use default security profile
                     1024*1024,      // use stack size in bytes (default 1 MB)
                     &pushpop2_pusher, // proc
                     (LPVOID) q,     // lpparam
                     CREATE_SUSPENDED,  // creation flags - set to 0 to run immediately                     
                     NULL)           // (output) thread id - NULL ignores
            };
  int i,nthreads = sizeof(threads)/sizeof(HANDLE);
  double x;
  TicTocTimer t;
  
  debug("Before\r\n");
  debug("\t          Refs: %u\r\n", q->ref_count);
  debug("\t    Q capacity: %d\r\n", q->q->ring->nelem );
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
  debug("After:\r\n");
  debug("\tElapsed: %f seconds\r\n",x);
  debug("\t    Q capacity: %d\r\n", q->q->ring->nelem );
  Guarded_Assert( q->ref_count == 1 );
  Guarded_Assert( q->waiting_producers == 0 );
  Guarded_Assert( q->waiting_consumers == 0 );  
  Guarded_Assert( Asynq_Unref(q) ); // Assert this is the last reference and is freed.
  
  while( nthreads-- )
    Guarded_Assert_WinErr( CloseHandle( threads[nthreads] ));
} 

//
// Pushpop3
// --------
//  - use no load producers and consumers
//  - use a peeker thread with no load
//  - use a peeker thread with some load to test
//    deadlocks from starved consumers
//

DWORD WINAPI pushpop3_pusher(LPVOID lpParam)
{ asynq *q = Asynq_Ref( (asynq*) lpParam );
  u8 *u,*buf = (u8*) Asynq_Token_Buffer_Alloc(q);
  int n = 100;
  unsigned int res = 1;
  testasynq_debug("pusher start\r\n");
  while(n--)
  { buf[0] = n;
    u = buf;
    res &= Asynq_Push(q, (void**) &buf, TRUE);    
    testasynq_debug("\t+%c 0x%p %5d       %5d\r\n",
                  Asynq_Is_Full(q)?'x':' ',
                  u,
                  n,
                  q->q->ring->nelem);
  }
  Asynq_Token_Buffer_Free(buf);
  testasynq_debug("pusher stop: %d\r\n",res);
  Asynq_Unref(q);   
  ExitThread(res==0);
  return 0;
}

DWORD WINAPI pushpop3_popper(LPVOID lpParam)
{ asynq *q = Asynq_Ref( (asynq*) lpParam );
  u8 *buf = (u8*) Asynq_Token_Buffer_Alloc(q);
  int n = 100;
  unsigned int res = 1;
  testasynq_debug("popper start\r\n");
  while(n--)
  { res &= Asynq_Pop(q,(void**)&buf);    
    testasynq_debug("\t-%c 0x%p %5d %5u\r\n",
                      Asynq_Is_Empty(q)?'x':' ',
                      res?buf:NULL,
                      n,
                      buf[0]);
  }
  Asynq_Token_Buffer_Free(buf);
  testasynq_debug("popper stop: %d\r\n",res);
  Asynq_Unref(q);
  ExitThread(res==0);
  return 0;
}

DWORD WINAPI pushpop3_peeker(LPVOID lpParam)
{ asynq *q = Asynq_Ref( (asynq*) lpParam );
  u8 *buf = (u8*) Asynq_Token_Buffer_Alloc(q);
  int n = 100;
  unsigned int res = 1;
  testasynq_debug("peeker start\r\n");
  while(n--)
  { res &= Asynq_Peek(q,(void*)buf);    
    testasynq_debug("\to%c 0x%p %5d %5u\r\n",
                      Asynq_Is_Empty(q)?'x':' ',
                      res?buf:NULL,
                      n,
                      buf[0]);    
  }
  Asynq_Token_Buffer_Free(buf);
  testasynq_debug("peeker stop: %d\r\n",res);
  Asynq_Unref(q);
  ExitThread(res==0);
  return 0;
}

DWORD WINAPI pushpop3_peeker_timed(LPVOID lpParam)
{ asynq *q = Asynq_Ref( (asynq*) lpParam );
  u8 *buf = (u8*) Asynq_Token_Buffer_Alloc(q);
  int n = 100;
  unsigned int res = 1;
  testasynq_debug("timed peeker start\r\n");
  while(n--)
  { res &= Asynq_Peek_Timed(q,(void*)buf, 100);
    testasynq_debug("\t=%c 0x%p %5d %5u\r\n",
                      Asynq_Is_Empty(q)?'x':' ',
                      res?buf:NULL,
                      n,
                      buf[0]);
    spin(2000000);
  }
  Asynq_Token_Buffer_Free(buf);
  testasynq_debug("timed peeker stop: %d\r\n",res);
  Asynq_Unref(q);
  ExitThread(res==0);
  return 0;
}

void pushpop3(size_t w, size_t h, size_t nchan, size_t bytes_per_pixel)
{ 
  asynq *q = Asynq_Alloc(4, w*h*nchan*bytes_per_pixel );
  HANDLE threads[] = {
             CreateThread( NULL,     // use default security profile
                     1024*1024,      // use stack size in bytes (default 1 MB)
                     &pushpop3_popper,  // proc
                     (LPVOID) q,     // lpparam
                     CREATE_SUSPENDED,  // creation flags - set to 0 to run immediately
                     NULL),          // (output) thread id - NULL ignores
                     
             CreateThread( NULL,     // use default security profile
                     1024*1024,      // use stack size in bytes (default 1 MB)
                     &pushpop3_peeker,  // proc
                     (LPVOID) q,     // lpparam
                     CREATE_SUSPENDED,  // creation flags - set to 0 to run immediately
                     NULL),          // (output) thread id - NULL ignores
                     
             CreateThread( NULL,     // use default security profile
                     1024*1024,      // use stack size in bytes (default 1 MB)
                     &pushpop3_peeker_timed,  // proc
                     (LPVOID) q,     // lpparam
                     CREATE_SUSPENDED,  // creation flags - set to 0 to run immediately
                     NULL),          // (output) thread id - NULL ignores
                     
             CreateThread( NULL,     // use default security profile
                     1024*1024,      // use stack size in bytes (default 1 MB)
                     &pushpop3_pusher,  // proc
                     (LPVOID) q,     // lpparam
                     CREATE_SUSPENDED,  // creation flags - set to 0 to run immediately                     
                     NULL)           // (output) thread id - NULL ignores
            };
  int i,nthreads = sizeof(threads)/sizeof(HANDLE);
  double x;
  TicTocTimer t;
  
  debug("Before\r\n");
  debug("\t          Refs: %u\r\n", q->ref_count);
  debug("\t    Q capacity: %d\r\n", q->q->ring->nelem );
  Guarded_Assert( q->ref_count == 1 );
  Guarded_Assert( q->waiting_consumers == 0);
  Guarded_Assert( q->waiting_producers == 0);
  
  t = tic();  
  for(i=0;i<nthreads;i++)
  { Guarded_Assert( threads[i] );
    ResumeThread( threads[i] );
  }
  
  WaitForMultipleObjects( nthreads, threads, TRUE, 1000 );  
  Asynq_Flush_Waiting_Consumers(q,100);
  WaitForMultipleObjects( nthreads, threads, TRUE, INFINITE );
  x = toc(&t);
  
  debug("After:\r\n");
  debug("\tElapsed: %f seconds\r\n",x);
  debug("\t          Refs: %u\r\n", q->ref_count);
  debug("\t    Q capacity: %d\r\n", q->q->ring->nelem );
  Guarded_Assert( q->ref_count == 1 );
  Guarded_Assert( q->waiting_producers == 0 );
  Guarded_Assert( q->waiting_consumers == 0 );  
  Guarded_Assert( Asynq_Unref(q) );
  
  while( nthreads-- )
    Guarded_Assert_WinErr( CloseHandle( threads[nthreads] ));
} 

//
// Pushspin1
// --------
//  - push with no expansion
//  - some load on producer thread (producer bottlenecked)
//  - one producer/one peeker (peek)

DWORD WINAPI pushspin1_pusher(LPVOID lpParam)
{ asynq *q = Asynq_Ref( (asynq*) lpParam );
  u8 *u,*buf = (u8*) Asynq_Token_Buffer_Alloc(q);
  int n = 100;
  unsigned int res = 1;
  testasynq_debug("pusher start\r\n");
  while(n--)
  { memset(buf,n,q->q->buffer_size_bytes);
    u = buf;
    res &= Asynq_Push(q, (void**) &buf, FALSE);    
    testasynq_debug("\t+%c 0x%p %5d       %5d\r\n",
                  Asynq_Is_Full(q)?'x':' ',
                  u,
                  n,
                  q->q->ring->nelem);
  }
  Asynq_Token_Buffer_Free(buf);
  testasynq_debug("pusher stop: %d\r\n",res);
  Asynq_Unref(q);   
  ExitThread(res==0);
  return 0;
}

DWORD WINAPI pushspin1_peeker(LPVOID lpParam)
{ asynq *q = Asynq_Ref( (asynq*) lpParam );
  u8 *buf = (u8*) Asynq_Token_Buffer_Alloc(q);
  int n = 100;
  unsigned int res = 1;
  testasynq_debug("peeker start\r\n");
  while(n--)
  { res &= Asynq_Peek(q,(void*)buf);    
    testasynq_debug("\t=%c 0x%p %5d %5u\r\n",
                      Asynq_Is_Empty(q)?'x':' ',
                      res?buf:NULL,
                      n,
                      buf[0]);
  }
  Asynq_Token_Buffer_Free(buf);
  testasynq_debug("peeker stop: %d\r\n",res);
  Asynq_Unref(q);
  ExitThread(res==0);
  return 0;
}

void pushspin1(size_t w, size_t h, size_t nchan, size_t bytes_per_pixel)
{ 
  asynq *q = Asynq_Alloc( 4, w*h*nchan*bytes_per_pixel );
  HANDLE threads[] = {
             CreateThread( NULL,     // use default security profile
                     1024*1024,      // use stack size in bytes (default 1 MB)
                     &pushspin1_pusher, // proc
                     (LPVOID) q,     // lpparam
                     CREATE_SUSPENDED,  // creation flags - set to 0 to run immediately
                     NULL),          // (output) thread id - NULL ignores
                     
             CreateThread( NULL,     // use default security profile
                     1024*1024,      // use stack size in bytes (default 1 MB)
                     &pushspin1_peeker, // proc
                     (LPVOID) q,     // lpparam
                     CREATE_SUSPENDED,  // creation flags - set to 0 to run immediately                     
                     NULL)           // (output) thread id - NULL ignores
            };
  int i,nthreads = sizeof(threads)/sizeof(HANDLE);
  double x;
  TicTocTimer t;
  
  debug("Before\r\n");
  debug("\t          Refs: %u\r\n", q->ref_count);
  debug("\tWaiting pushes: %u\r\n", q->waiting_producers );
  debug("\tWaiting  peeks: %u\r\n", q->waiting_consumers );
  debug("\t            &Q: 0x%p\r\n", q->q );
  debug("\t    Q capacity: %d\r\n", q->q->ring->nelem );
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
  debug("After:\r\n");
  debug("\tElapsed: %f seconds\r\n",x);
  debug("\t          Refs: %u\r\n", q->ref_count);
  debug("\tWaiting pushes: %u\r\n", q->waiting_producers );
  debug("\tWaiting  peeks: %u\r\n", q->waiting_consumers );
  debug("\t    Q capacity: %d\r\n", q->q->ring->nelem );
  Guarded_Assert( q->ref_count == 1 );
  Guarded_Assert( q->waiting_producers == 0 );
  Guarded_Assert( q->waiting_consumers == 0 );  
  Guarded_Assert( Asynq_Unref(q) );
  
  while( nthreads-- )
    Guarded_Assert_WinErr( CloseHandle( threads[nthreads] ));  
}

// ----
// MAIN
// ----                                                       

int _tmain(int argc, _TCHAR* argv[])
{ Reporting_Setup_Log_To_Stdout();
  Reporting_Setup_Log_To_VSDebugger_Console();
 
  debug("---  pushpop1 ---------------------\r\n");
  pushpop1( 1024,1024,8,2 );
  debug("---  pushpop2 ---------------------\r\n");
  pushpop2( 1024,1024,8,2 );
  debug("---  pushpop3 ---------------------\r\n");
  pushpop3( 1024,1024,8,2 );
  debug("---  pushspin1 --------------------\r\n");
  pushspin1( 1024,1024,8,2 );
  
	return 0;
}

