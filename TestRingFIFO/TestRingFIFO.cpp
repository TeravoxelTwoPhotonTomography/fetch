// TestRingFIFO.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "../fetch/ring-fifo.h"


void create_destroy(size_t w, size_t h, size_t nchan, size_t bytes_per_pixel)
{ size_t sz = w*h*nchan*bytes_per_pixel;
  int maxbuf = 256,
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
  double delta;
  TicTocTimer t;
  RingFIFO *r = RingFIFO_Alloc( maxbuf, sz); 
  void *buf   = RingFIFO_Alloc_Token_Buffer(r);

  debug("test ring fifo : Pushes with no expansion\r\n"
        "\tCreating/Detroying up to %5d buffers.  Each: %u bytes.\r\n"
        "\t\twidth:      %5u\r\n"
        "\t\theight:     %5u\r\n"
        "\t\tchannels:   %5u\r\n"
        "\t\tbyte depth: %5u\r\n"
        "\r\n"
        "\t\t  iter   time(ms)  head  tail  sz  e  f\r\n"
        "\t\t-------  --------  ----  ----  --  -  -\r\n", 
        maxbuf, sz, w, h, nchan, bytes_per_pixel);

  t = tic();
  for( i=1; i<iter; i++ )
  { RingFIFO_Push( r, buf, FALSE );
    debug("\t\t%6d  %8.3f %4d %4d %2d %c %c\r\n",
           iter, toc(&t), r->head, r->tail, r->head - r->tail,
           RingFIFO_Is_Empty(r)?'x':' ',
           RingFIFO_Is_Full(r) ?'x':' ');
  }
  RingFIFO_Free(r);
  free(buf);
}

void pushes_w_exp(size_t w, size_t h, size_t nchan, size_t bytes_per_pixel)
{ size_t sz = w*h*nchan*bytes_per_pixel;
  int maxbuf = 2,
      iter   = 1239,
      i;
  double delta;
  TicTocTimer t;
  RingFIFO *r = RingFIFO_Alloc( maxbuf, sz); 
  void *buf   = RingFIFO_Alloc_Token_Buffer(r);

  debug("test ring fifo : Pushes with expansion\r\n"
        "\tCreating/Detroying up to %5d buffers.  Each: %u bytes.\r\n"
        "\t\twidth:      %5u\r\n"
        "\t\theight:     %5u\r\n"
        "\t\tchannels:   %5u\r\n"
        "\t\tbyte depth: %5u\r\n"
        "\r\n"
        "\t\t  iter   time(ms)  head  tail  sz  e  f\r\n"
        "\t\t-------  --------  ----  ----  --  -  -\r\n", 
        maxbuf, sz, w, h, nchan, bytes_per_pixel);

  t = tic();
  for( i=1; i<iter; i++ )
  { RingFIFO_Push( r, buf, TRUE );
    debug("\t\t%6d  %8.3f %4d %4d %2d %c %c\r\n",
           iter, toc(&t), r->head, r->tail, r->head - r->tail,
           RingFIFO_Is_Empty(r)?'x':' ',
           RingFIFO_Is_Full(r) ?'x':' ');
  }
  RingFIFO_Free(r);
  free(buf);
}
#if 0
void pushes_w_exp()
{
}

void pushes_try()
{
}

void pops()
{
}

void peeks()
{
}
#endif

int _tmain(int argc, _TCHAR* argv[])
{ Reporting_Setup_Log_To_Stdout();
  Reporting_Setup_Log_To_VSDebugger_Console();
  
  create_destroy(1024,1024,8,2);
  create_destroy(512,512,8,2);
  pushes_no_exp(1024,1024,8,2);
  pushes_w_exp(1024,1024,8,2);
  
	return 0;
}

