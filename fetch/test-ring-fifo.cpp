#include "ring-fifo.h"

void test_ring_fifo_create_destroy()
{ size_t w     = 1024,
         h     = 1024,
         nchan = 8,
         bytes_per_pixel = 2,
         sz = w*h*nchan*bytes_per_pixel;
  int maxbuf = 100,
      i;
  debug("test_ring_fifo_create_destroy\r\n"
        "\tCreating/Detroying up to %5d buffers.  Each: %u bytes.\r\n"
        "\t\twidth:      %5u\r\n"
        "\t\theight:     %5u\r\n"
        "\t\tchannels:   %5u\r\n"
        "\t\tbyte depth: %5u\r\n", maxbuf, sz, w, h, nchan, bytes_per_pixel);
  for( i=0; i<maxbuf; i++ )
  { RingFIFO *r = RingFIFO_Alloc( i, sz);
    RingFIFO_Free(r);
  }
}