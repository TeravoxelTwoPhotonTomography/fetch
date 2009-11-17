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

#if 0
void pushes_no_exp()
{
}

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
  
	return 0;
}

