#include "stdafx.h"
#include "frame2.h"

/*
 * MESSAGE
 */

void
Message::to_file(FILE *fp)                                     // Writing to a file/byte stream                                           
{ size_t sz  = this->size_bytes(),                             // -----------------------------                                           
         off = (u8*)this->data - (u8*)this;                    // 0. Compute the size of the formating data
  Guarded_Assert( 1 == fwrite( &sz,  sizeof(size_t), 1, fp )); // 1. The size (in bytes) is written to the stream.
  Guarded_Assert( 1 == fwrite( &off, sizeof(size_t), 1, fp )); // 2. Write out the size of the formating data.
  Guarded_Assert( 1 == fwrite( this, 1, sz, fp ));             // 3. A block of data from <this> to <this+size> is written to the stream. 
}

size_t
Message::from_file(FILE *fp, Message* workspace, size_t size_workspace)
{ size_t sz,off;
  fpos_t bookmark;
  Guarded_Assert( 0 == fgetpos( fp, &bookmark ) );
  Guarded_Assert( 1 == fread( &sz,  sizeof(size_t), 1, fp ));

  if( workspace &&  size_workspace < sz )
  { Guarded_Assert( 0 == fsetpos( fp, &bookmark ) ); // reset the stream position
    return sz;                                       // failure.  Indicates the required storage size.
  }

  Guarded_Assert( 1 == fread( &off, sizeof(size_t), 1, fp ));
  Guarded_Assert( sz == fread( workspace, 1, sz, fp ) );
  workspace->data = (u8*)workspace + off;
  return 0; // Success.
}

/*
 * FRAME
 */

Frame::Frame(u16 width, u16 height, u8 nchan, Basic_Type_ID type)
      : width(width),
        height(height),
        nchan(nchan),
        type(type),
        Bpp( g_type_attributes[type].bytes )
{}

size_t
Frame::size_bytes(void)
{ return sizeof(this) + this->width * 
                        this->height*
                        this->nchan *
                        this->Bpp;
}

/*
 * Frame_With_Interleaved_Pixels
 *
 * TODO:
 * [ ] static  Message *translate    ( Message *src );
 */

Frame_With_Interleaved_Pixels::
  Frame_With_Interleaved_Pixels(u16 width, u16 height, u8 nchan, Basic_Type_ID type)
                              : width(width),
                                height(height),
                                nchan(nchan),
                                type(type),
                                Bpp( g_type_attributes[type].bytes ),
                                id( FRAME_INTERLEAVED_PIXELS )
{}

void Frame_With_Interleaved_Pixels::
copy_channel( void *dst, size_t rowpitch, size_t ichan )
{ size_t irow = this->height,
          Bpp = this->Bpp,
         step = this->nchan * Bpp,
            w = this->width * step,
        nelem = MIN( rowpitch/step, this->width );
  void   *src = this->data,
  while(irow--)
  { size_t n = nelem;
    void *dstcur = dst + irow * rowpitch + n * step,
         *srccur = src + irow * w        + n * step;
    while( n-- )
    { dstcur -= step;
      srccur -= step;
      memcpy( dstcur, srccur, step );
    }
  }
}


/*
 * Frame_With_Interleaved_Planes
 *
 * TODO:
 * [ ] static  Message *translate    ( Message *src );
 */

Frame_With_Interleaved_Planes::
  Frame_With_Interleaved_Planes(u16 width, u16 height, u8 nchan, Basic_Type_ID type)
                              : width(width),
                                height(height),
                                nchan(nchan),
                                type(type),
                                Bpp( g_type_attributes[type].bytes ),
                                id( FRAME_INTERLEAVED_PLANES)
{}

void Frame_With_Interleaved_Pixels::
copy_channel( void *dst, size_t rowpitch, size_t ichan )
{ size_t irow = this->height,
            w = this->width * this->Bpp;
  void   *src = this->data,
  while(irow--)
    memcpy( dst + irow * rowpitch, src + irow * w, w );
}
