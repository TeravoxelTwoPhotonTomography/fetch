// TestRingFIFO.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "../fetch/ring-fifo.h"
#include <math.h>  // for sqrt
#include <float.h> // for DBL_MAX

void create_destroy(size_t w, size_t h, size_t nchan, size_t bytes_per_pixel)
{ size_t sz = w*h*nchan*bytes_per_pixel;
  int maxbuf = 128,
      i;
  double delta;
  TicTocTimer t;
  debug("test_ring_fifo_create_destroy\r\n"
        "\tCreating/Detroying up to %5d buffers.  Each: %u bytes.\r\n"
        "\t\twidth:      %5u\r\n"
        "\t\theight:     %5u\r\n"
        "\t\tchannels:   %5u\r\n"
        "\t\tbyte depth: %5u\r\n"
        "\r\n"
        "\t\t#bufs  size(MB)  time(ms)   MB/ms \r\n"
        "\t\t-----  --------  --------  -------\r\n", 
        maxbuf, sz, w, h, nchan, bytes_per_pixel);
  t = tic();
  for( i=1; i<maxbuf; i = i<<1 )
  { RingFIFO *r = RingFIFO_Alloc( i, sz);
    RingFIFO_Free(r);
    debug("\t\t%5d  %8.3f  %8.3f  %7.3f\r\n",
          i, 
          i*sz*1.0e-6, 
          (delta=toc(&t))*1e3,
          1e-9*i*sz/delta );
  }
}

void pushes_no_exp(size_t w, size_t h, size_t nchan, size_t bytes_per_pixel)
{ size_t sz = w*h*nchan*bytes_per_pixel;
  int maxbuf = 32,
      iter   = 5*maxbuf,
      i;  
  TicTocTimer t;
  RingFIFO *r = RingFIFO_Alloc( maxbuf, sz); 
  void *buf   = RingFIFO_Alloc_Token_Buffer(r);

  debug("test ring fifo : Pushes with no expansion\r\n"
        "\tPushing up to %5d buffers.  Each: %u bytes.\r\n"
        "\t\twidth:      %5u\r\n"
        "\t\theight:     %5u\r\n"
        "\t\tchannels:   %5u\r\n"
        "\t\tbyte depth: %5u\r\n"
        "\r\n"
        "\t\t  iter   time(us)  head  tail  e  f  sz\r\n"
        "\t\t-------  --------  ----  ----  -  -  --\r\n", 
        maxbuf, sz, w, h, nchan, bytes_per_pixel);

  t = tic();
  for( i=1; i<iter; i++ )
  { toc(&t);
    RingFIFO_Push( r, &buf, FALSE );
    debug("\t\t%6d  %8.1f  % 4d  % 4d   %c  %c  %2d\r\n",
           i, 1e6*toc(&t), r->head, r->tail,
           RingFIFO_Is_Empty(r)?'x':'.',
           RingFIFO_Is_Full(r) ?'x':'.', r->head - r->tail);
  }
  RingFIFO_Free(r);
  free(buf);
}

void time_pushes_no_exp(size_t w, size_t h, size_t nchan, size_t bytes_per_pixel)
{ size_t sz = w*h*nchan*bytes_per_pixel;
  int maxbuf = 32,
      inner  = 10*maxbuf,
      outer  = 30,
      i,j;  
  TicTocTimer t;
  double  x,
        acc   =  0.0, 
        acc2  =  0.0, 
        max   = -DBL_MAX,
        min   =  DBL_MAX,
        N     =  outer;  
  unsigned zeros = 0;
  RingFIFO *r = RingFIFO_Alloc( maxbuf, sz); 
  void *buf   = RingFIFO_Alloc_Token_Buffer(r);

  debug("test ring fifo : Time pushes with no expansion\r\n"
        "\tPushing up to %5d buffers.  Each: %u bytes.\r\n"
        "\t\twidth:      %5u\r\n"
        "\t\theight:     %5u\r\n"
        "\t\tchannels:   %5u\r\n"
        "\t\tbyte depth: %5u\r\n"
        "\r\n", 
        inner, sz, w, h, nchan, bytes_per_pixel);

  t = tic();
  for( j=0; j<outer; j++)
  { for( i=0; i<inner; i++ )
      RingFIFO_Push( r, &buf, FALSE );   
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
        "min :    %g us\r\n"
        "FPS :    %g buffers/sec.\r\n", 100.0*zeros/N,
                              acc  / N,
                              sqrt( acc2 / N - (acc/N)*(acc/N) ),
                              max,
                              min,
                              1e6*N*inner/acc);
  RingFIFO_Free(r);
  free(buf);
}

void pushes_w_exp(size_t w, size_t h, size_t nchan, size_t bytes_per_pixel)
{ size_t sz = w*h*nchan*bytes_per_pixel;
  int maxbuf = 2,
      iter   = 64,
      i;
  TicTocTimer t;
  RingFIFO *r = RingFIFO_Alloc( maxbuf, sz); 
  void *buf   = RingFIFO_Alloc_Token_Buffer(r);

  debug("test ring fifo : Pushes with expansion\r\n"
        "\tPushing up to %5d buffers.  Each: %u bytes.\r\n"
        "\t\twidth:      %5u\r\n"
        "\t\theight:     %5u\r\n"
        "\t\tchannels:   %5u\r\n"
        "\t\tbyte depth: %5u\r\n"
        "\r\n"
        "\t\t  iter   time(us)  head  tail  e  f  sz\r\n"
        "\t\t-------  --------  ----  ----  -  -  --\r\n", 
        maxbuf, sz, w, h, nchan, bytes_per_pixel);

  t = tic();
  for( i=1; i<iter; i++ )
  { toc(&t);
    RingFIFO_Push( r, &buf, TRUE );
    debug("\t\t%6d  %8.1f  % 4d  % 4d   %c  %c  %2d\r\n",
           i, 1e6*toc(&t), r->head, r->tail,
           RingFIFO_Is_Empty(r)?'x':'.',
           RingFIFO_Is_Full(r) ?'x':'.', r->head - r->tail);
  }
  RingFIFO_Free(r);
  free(buf);
}

void pushes_try(size_t w, size_t h, size_t nchan, size_t bytes_per_pixel)
{ size_t sz = w*h*nchan*bytes_per_pixel;
  int maxbuf = 32,
      iter   = 5*maxbuf,
      i;  
  TicTocTimer t;
  RingFIFO *r = RingFIFO_Alloc( maxbuf, sz); 
  void *buf   = RingFIFO_Alloc_Token_Buffer(r);

  debug("test ring fifo : Try Pushes\r\n"
        "Pushing up to %5d buffers.  Each: %u bytes.\r\n"
        "\t\twidth:      %5u\r\n"
        "\t\theight:     %5u\r\n"
        "\t\tchannels:   %5u\r\n"
        "\t\tbyte depth: %5u\r\n"
        "\r\n"
        "\t\t  iter   time(us)  head  tail  e  f  sz\r\n"
        "\t\t-------  --------  ----  ----  -  -  --\r\n", 
        maxbuf, sz, w, h, nchan, bytes_per_pixel);

  t = tic();
  for( i=1; i<iter; i++ )
  { toc(&t);
    RingFIFO_Push_Try( r, &buf );
    debug("\t\t%6d  %8.1f  % 4d  % 4d   %c  %c  %2d\r\n",
           i, 1e6*toc(&t), r->head, r->tail,
           RingFIFO_Is_Empty(r)?'x':'.',
           RingFIFO_Is_Full(r) ?'x':'.', r->head - r->tail);
  }
  RingFIFO_Free(r);
  free(buf);
}

void pushpop1(size_t w, size_t h, size_t nchan, size_t bytes_per_pixel)
{ size_t sz = w*h*nchan*bytes_per_pixel;
  RingFIFO *r = RingFIFO_Alloc( 1, sz); 
  void * in   = RingFIFO_Alloc_Token_Buffer(r),
       *out   = RingFIFO_Alloc_Token_Buffer(r),
       *old   = in;
  RingFIFO_Push(r,&in,1);
  RingFIFO_Pop(r,&out);
  Guarded_Assert(old==out);

  old = in;
  RingFIFO_Push(r,&in,1);
  RingFIFO_Push(r,&in,1);
  RingFIFO_Pop(r,&out);
  Guarded_Assert(old==out);
  Guarded_Assert(r->ring->nelem==2);
  RingFIFO_Pop(r,&out);   // empty
  
  old = in;
  RingFIFO_Push(r,&in,1);
  RingFIFO_Push(r,&in,1);
  RingFIFO_Push(r,&in,1);
  RingFIFO_Pop(r,&out);
  Guarded_Assert(old==out);
  Guarded_Assert(r->ring->nelem==4);
  RingFIFO_Pop(r,&out);
  RingFIFO_Pop(r,&out);   // empty
  
  RingFIFO_Push(r,&in,1);
  RingFIFO_Push(r,&in,1);
  RingFIFO_Push(r,&in,1);
  old = in;
  RingFIFO_Push(r,&in,1);
  RingFIFO_Push(r,&in,1);
  RingFIFO_Pop(r,&out);
  RingFIFO_Pop(r,&out);
  RingFIFO_Pop(r,&out);
  RingFIFO_Pop(r,&out);
  Guarded_Assert(old==out);
  Guarded_Assert(r->ring->nelem==8);  
  RingFIFO_Pop(r,&out);   // empty
  
  free( in );
  free( out);
  RingFIFO_Free(r);       
  
}

void pushpops(size_t w, size_t h, size_t nchan, size_t bytes_per_pixel)
{ size_t sz = w*h*nchan*bytes_per_pixel;
  int maxbuf = 2,
      iter   = 64,
      i;
  TicTocTimer t;
  RingFIFO *r = RingFIFO_Alloc( maxbuf, sz); 
  void *buf   = RingFIFO_Alloc_Token_Buffer(r);
  int onoff[] = {15,12,18, 7,18, 1, 1, 1,
                  1, 1, 1, 1, 1, 1, 1,48};
  int expand[] ={ 1, 0, 0, 0, 1, 1, 1, 1,
                  1, 1, 1, 1, 1, 1, 1, 1};
  int n = sizeof(onoff)/sizeof(int),
      j;

  debug("test ring fifo : Pushes with expansion\r\n"
        "\tStarting with %5d buffers.  Each: %u bytes.\r\n"
        "\t\twidth:      %5u\r\n"
        "\t\theight:     %5u\r\n"
        "\t\tchannels:   %5u\r\n"
        "\t\tbyte depth: %5u\r\n"
        "\r\n"
        "\t\t     iter   time(us)  head  tail  e  f  sz\r\n"
        "\t\t   -------  --------  ----  ----  -  -  --\r\n", 
        maxbuf, sz, w, h, nchan, bytes_per_pixel);
                
  for(j=0;j<n;j++)
  { t = tic();
    for( i=1; i<=onoff[j]; i++ )
    { void *u;
      toc(&t);
      if(j&1)
      { u = buf; RingFIFO_Pop( r, &buf); }
      else
      { RingFIFO_Push( r, &buf, expand[j] ); u = buf;}
      debug("\t\t%c  %6d  %8.1f  % 4d  % 4d   %c  %c  %2d 0x%p\r\n",
             (j&1)?'-':'+',
             i, 1e6*toc(&t), r->head, r->tail,
             RingFIFO_Is_Empty(r)?'x':'.',
             RingFIFO_Is_Full(r) ?'x':'.', 
             r->head - r->tail,
             u);
    }
  }
  RingFIFO_Free(r);
  free(buf);  
}

void time_pushpops(size_t w, size_t h, size_t nchan, size_t bytes_per_pixel)
{ size_t sz = w*h*nchan*bytes_per_pixel;
  int maxbuf = 64,
      inner  = maxbuf,
      outer  = 30,
      i,j;  
  TicTocTimer t;
  double  x,
        acc   =  0.0, 
        acc2  =  0.0, 
        max   = -DBL_MAX,
        min   =  DBL_MAX,
        N     =  outer;  
  unsigned zeros = 0;
  RingFIFO *r = RingFIFO_Alloc( maxbuf, sz); 
  void *buf   = RingFIFO_Alloc_Token_Buffer(r);

  debug("test ring fifo : Timed push/pops\r\n"
        "\tPushing then popping %5d buffers.  Each: %u bytes.\r\n"
        "\t\twidth:      %5u\r\n"
        "\t\theight:     %5u\r\n"
        "\t\tchannels:   %5u\r\n"
        "\t\tbyte depth: %5u\r\n"
        "\r\n", 
        maxbuf, sz, w, h, nchan, bytes_per_pixel);

  t = tic();
  for( j=0; j<outer; j++)
  { for( i=0; i<inner; i++ )
      RingFIFO_Push( r, &buf, TRUE );
    for( i=0; i<inner; i++ )
      RingFIFO_Pop( r, &buf );
    x = toc(&t)*1e6; // convert to microseconds
    acc  += x;
    acc2 += x*x;
    max   = MAX(max,x);
    min   = MIN(min,x);
    zeros += (x<1e-6); // count differences less than a picosecond    
  }
  debug("zeros:   %f %%\r\n"
        "mean :   %g us\r\n"
        "std :    %g us\r\n"
        "max :    %g us\r\n"
        "min :    %g us\r\n"
        "FPS :    %g buffers/sec.\r\n", 100.0*zeros/N,
                              acc  / N,
                              sqrt( acc2 / N - (acc/N)*(acc/N) ),
                              max,
                              min,
                              1e6*N*inner/acc);                               
  RingFIFO_Free(r);
  free(buf);
}

void time_peeks(size_t w, size_t h, size_t nchan, size_t bytes_per_pixel)
{ size_t sz = w*h*nchan*bytes_per_pixel;
  int maxbuf = 16,
      inner  = 16,
      outer  = 64,
      i,j;
  TicTocTimer t;
  RingFIFO *r = RingFIFO_Alloc( maxbuf, sz); 
  void *buf   = RingFIFO_Alloc_Token_Buffer(r);
  double  x,
        acc   =  0.0, 
        acc2  =  0.0, 
        max   = -DBL_MAX,
        min   =  DBL_MAX,
        N     =  outer;  
  unsigned zeros = 0;

  debug("test ring fifo : Peeks\r\n"
        "\tStarting with %5d buffers.  Each: %u bytes.\r\n"
        "\t\twidth:      %5u\r\n"
        "\t\theight:     %5u\r\n"
        "\t\tchannels:   %5u\r\n"
        "\t\tbyte depth: %5u\r\n", 
        maxbuf, sz, w, h, nchan, bytes_per_pixel);
        
  Guarded_Assert( RingFIFO_Peek( r, buf) ); // should return indicating empty
  
  for( i=0; i<maxbuf; i++ )
      RingFIFO_Push( r, &buf, TRUE);
      
  Guarded_Assert( !RingFIFO_Peek( r, buf) ); // should return indicating not empty
      
  for(j=0;j<outer;j++)
  { t = tic();
    for(i=0;i<inner;i++)
      RingFIFO_Peek(r,buf);
    x = toc(&t)*1e3; // convert to milliseconds
    acc  += x;
    acc2 += x*x;
    max   = MAX(max,x);
    min   = MIN(min,x);
    zeros += (x<1e-9); // count differences less than a picosecond 
  }
  debug("zeros:   %f %%\r\n"
      "mean :   %g ms\r\n"
      "std :    %g ms\r\n"
      "max :    %g ms\r\n"
      "min :    %g ms\r\n"
      "FPS :    %g buffers/sec.\r\n", 100.0*zeros/N,
                            acc  / N,
                            sqrt( acc2 / N - (acc/N)*(acc/N) ),
                            max,
                            min,
                            1e3*N*inner/acc);  
  RingFIFO_Free(r);
  free(buf);  
}
int _tmain(int argc, _TCHAR* argv[])
{ Reporting_Setup_Log_To_Stdout();
  Reporting_Setup_Log_To_VSDebugger_Console();
  
  // CASES
  //pushes_no_exp(1024,1024,8,2);
  //pushes_w_exp(1024,1024,8,2);
  //pushes_try(1024,1024,8,2);
  pushpop1(1024,1024,8,2);
  pushpops(1024,1024,8,2);  
  
  // TIMING
  //create_destroy(1024,1024,8,2);
  //time_pushes_no_exp(1024,1024,8,2);
  //time_pushpops(1024,1024,8,2);
  //time_peeks(1024,1024,8,2);
  
	return 0;
}

