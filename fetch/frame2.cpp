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

size_t
Message::translate( Message *dst, Message *src )
{ size_t sz = src->size_bytes();
  if(dst) memcpy(dst,src,sz);
  return sz;
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

void 
Frame_With_Interleaved_Pixels::
  copy_channel( void *dst, size_t rowpitch, size_t ichan )
{ size_t pp = this->Bpp,
         dstw = rowpitch/pp,
         shape[] = {1,
                    MIN( this->width, dstw ),
                    this->height},
         dst_pitch[] = { rowpitch * this->height * pp,
                                        rowpitch * pp,
                                                   pp,
                                                   pp},
         src_pitch[] = {this->width * this->height * this->nchan * pp,
                        this->width *                this->nchan * pp,
                                                     this->nchan * pp,
                                                                   pp};
  imCopy<u8,u8>(dst,                       dst_pitch,
                src + ichan * src_pitch[3],src_pitch,
                shape);
}

/*
 * Frame_With_Interleaved_Planes
 *
 * TODO:
 * [ ] static  Message *translate    ( Message *src );
 */

Frame_With_Interleaved_Lines::
  Frame_With_Interleaved_Liness(u16 width, u16 height, u8 nchan, Basic_Type_ID type)
                              : width(width),
                                height(height),
                                nchan(nchan),
                                type(type),
                                Bpp( g_type_attributes[type].bytes ),
                                id( FRAME_INTERLEAVED_LINES )
{}

void Frame_With_Interleaved_Lines::
copy_channel( void *dst, size_t rowpitch, size_t ichan )
{ size_t pp = this->Bpp,
         dstw = rowpitch/pp,
         shape[] = {1,
                    MIN( this->width, dstw ),
                    this->height},
         dst_pitch[] = { rowpitch * this->height * pp,
                                        rowpitch * pp,
                                                   pp,
                                                   pp},
         src_pitch[] = {this->width * this->height * this->nchan * pp,
                        this->width *                this->nchan * pp,
                        this->width *                              pp,
                                                                   pp};
  imCopy<u8,u8>(dst,                       dst_pitch,
                src + ichan * src_pitch[2],src_pitch,
                shape);
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
                                id( FRAME_INTERLEAVED_PLANES )
{}

void Frame_With_Interleaved_Planes::
copy_channel( void *dst, size_t rowpitch, size_t ichan )
{ size_t pp = this->Bpp,
         dstw = rowpitch/pp,
         shape[] = {1,
                    MIN( this->width, dstw ),
                    this->height},
         dst_pitch[] = { rowpitch * this->height * pp,
                                        rowpitch * pp,
                                                   pp,
                                                   pp},
         src_pitch[] = {this->width * this->height * this->nchan * pp,
                        this->width * this->height *               pp,
                        this->width *                              pp,
                                                                   pp};
  imCopy<u8,u8>(dst,                       dst_pitch,
                src + ichan * src_pitch[1],src_pitch,
                shape);
}
