#include "stdafx.h"
#include "frame.h"
#include "util-file.h"
#include "util-image.h"

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

void
Message::to_file(HANDLE hfile)
{ size_t sz  = this->size_bytes(),
         off = (u8*)this->data - (u8*)this;
  DWORD  written;
  Guarded_Assert_WinErr( WriteFile( hfile, &sz,  sizeof(size_t), &written, NULL ));
  Guarded_Assert_WinErr( WriteFile( hfile, &off, sizeof(size_t), &written, NULL ));
  Guarded_Assert_WinErr( WriteFile( hfile, this,             sz, &written, NULL ));
}

size_t
Message::from_file(FILE *fp, Message* workspace, size_t size_workspace)
{ size_t sz,off;
  fpos_t bookmark;
  Guarded_Assert( 0 == fgetpos( fp, &bookmark ) );
  Guarded_Assert( 1 == fread( &sz,  sizeof(size_t), 1, fp ));

  if( !workspace || size_workspace >= sz )
  { Guarded_Assert( 0 == fsetpos( fp, &bookmark ) ); // reset the stream position
    return sz;                                       // failure.  Indicates the required storage size.
  }

  Guarded_Assert( 1 == fread( &off, sizeof(size_t), 1, fp ));
  Guarded_Assert( sz == fread( workspace, 1, sz, fp ) );
  workspace->data = (u8*)workspace + off;
  return 0; // Success.
}

size_t
Message::from_file(HANDLE hfile, Message* workspace, size_t size_workspace)
{ size_t sz,off;
  DWORD  bytes_read;
  int64  bookmark = w32file::getpos(hfile);
  Guarded_Assert_WinErr(    ReadFile( hfile,      &sz, sizeof(size_t), &bytes_read, NULL ));  

  if( !workspace ||  size_workspace >= sz )
  { w32file::setpos(hfile,bookmark,FILE_BEGIN);
    return sz;                                       // failure.  Indicates the required storage size.
  }

  Guarded_Assert_WinErr(    ReadFile( hfile,     &off, sizeof(size_t), &bytes_read, NULL ));  
  Guarded_Assert_WinErr(    ReadFile( hfile, worspace,             sz, &bytes_read, NULL ));  
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

Frame::Frame()
      : width(0),
        height(0),
        nchan(0),
        type(id_unspecified),
        Bpp(0),
        self_size(sizeof(Frame))
{}

Frame::Frame(u16 width, u16 height, u8 nchan, Basic_Type_ID type)
      : width(width),
        height(height),
        nchan(nchan),
        type(type),
        Bpp( g_type_attributes[type].bytes ),
        self_size(sizeof(Frame))
{}

unsigned int
Frame::is_equivalent( Frame *ref )
{ if( (ref->id     != this->id      ) ||
      (ref->width  != this->width   ) ||
      (ref->height != this->height  ) ||
      (ref->nchan  != this->nchan   ) ||
      (ref->rtti   != this->rtti    ) ) return 1;
  return 0;
}
    
void dump( const char *filename )
{ size_t  n = this->width * 
              this->height*
              this->nchan;
  FILE *fp = fopen(filename,"wb");
  fwrite(this->data, this->Bpp, n, fp );
  fclose(fp);
}

size_t
Frame::size_bytes(void)
{ return this->self_size + this->width * 
                           this->height*
                           this->nchan *
                           this->Bpp;
}

void
Frame::format(Message* unformatted)
{ memcpy( unformatted, this, this->self_size );      // Copy the format header
  unformatted->data = unformatted + this->self_size; // Set the data section
}

/*
 * Frame_With_Interleaved_Pixels
 *
 */

Frame_With_Interleaved_Pixels::
  Frame_With_Interleaved_Pixels(u16 width, u16 height, u8 nchan, Basic_Type_ID type)
                              : width(width),
                                height(height),
                                nchan(nchan),
                                type(type),
                                Bpp( g_type_attributes[type].bytes ),
                                self_size(sizeof(Frame_With_Interleaved_Pixels)),
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
         dst_pitch[4], src_pitch[4];
  Compute_Pitch( dst_pitch, pp,           1,        dstw, this->height );
  Compute_Pitch( src_pitch, pp, this->nchan, this->width, this->height );
  imCopy<u8,u8>(dst,                        dst_pitch,
                src + ichan * src_pitch[3], src_pitch,
                shape);
}

size_t
Frame_With_Interleaved_Pixels::
  translate( Message *dst, Message *src )
{ size_t sz = src->size_bytes();

  switch( src->id )
  { case FRAME_INTERLEAVED_PIXELS:
      return Message::translate(dst,src);
      break;
    case FRAME_INTERLEAVED_LINES:
      if(!dst) return sz;
      src->format(dst);                   // Format metadata is the same so just copy it in
      dst->id = FRAME_INTERLEAVED_PIXELS; // Set the format tag properly
      { size_t pp = src->Bpp,
               shape[] = { src->width, src->nchan, src->height },
               dst_pitch[4],
               src_pitch[4];
        Compute_Pitch( dst_pitch, pp, src->nchan, src->width, src->height );
        Compute_Pitch( src_pitch, pp, src->width, src->nchan, src->height );
        imCopyTranspose<u8,u8>( dst->data, dst_pitch, 
                                src->data, src_pitch, shape, 0, 1 ); 
        
      }
      break;
    case FRAME_INTERLEAVED_PLANES:
      if(!dst) return sz;
      src->format(dst);                   // Format metadata is the same so just copy it in
      dst->id = FRAME_INTERLEAVED_PIXELS; // Set the format tag properly
      { size_t pp = src->Bpp,
               shape[] = { src->width, src->height, src->nchan },
               dst_pitch[4],
               src_pitch[4];
        // This ends up apparently transposing width and height.
        // Really, doing this properly requires two transposes.
        Compute_Pitch( dst_pitch, pp, src->nchan, src->height, src->width );
        Compute_Pitch( src_pitch, pp, src->width, src->height, src->nchan  );
        imCopyTranspose<u8,u8>( dst->data, dst_pitch, 
                                src->data, src_pitch, shape, 0, 2 ); 
        
      }
      break;
    default:
      warning("Failed attempt to translate a message.");
      return 0; // no translation
  }
  return sz;
}

/*
 * Frame_With_Interleaved_Planes
 *
 */

Frame_With_Interleaved_Lines::
  Frame_With_Interleaved_Liness(u16 width, u16 height, u8 nchan, Basic_Type_ID type)
                              : width(width),
                                height(height),
                                nchan(nchan),
                                type(type),
                                Bpp( g_type_attributes[type].bytes ),
                                self_size(sizeof(Frame_With_Interleaved_Lines)),
                                id( FRAME_INTERLEAVED_LINES )
{}

void Frame_With_Interleaved_Lines::
copy_channel( void *dst, size_t rowpitch, size_t ichan )
{ size_t pp = this->Bpp,
         dstw = rowpitch/pp,
         shape[] = {1,
                    MIN( this->width, dstw ),
                    this->height},
         dst_pitch[],
         src_pitch[];
  Compute_Pitch( dst_pitch, pp,        dstw,           1, this->height );
  Compute_Pitch( src_pitch, pp, this->width, this->nchan, this->height );
  imCopy<u8,u8>(dst,                       dst_pitch,
                src + ichan * src_pitch[2],src_pitch,
                shape);
}

size_t
Frame_With_Interleaved_Lines::
  translate( Message *dst, Message *src )
{ size_t sz = src->size_bytes();

  switch( src->id )
  { case FRAME_INTERLEAVED_LINES:
      return Message::translate(dst,src);
      break;
    case FRAME_INTERLEAVED_PIXELS:
      if(!dst) return sz;
      src->format(dst);                   // Format metadata is the same so just copy it in
      dst->id = FRAME_INTERLEAVED_LINES;  // Set the format tag properly 
      { size_t pp = src->Bpp,
               shape[] = { src->nchan, src->width, src->height },
               dst_pitch[4],
               src_pitch[4];
        Compute_Pitch( dst_pitch, pp, src->width, src->nchan, src->height );
        Compute_Pitch( src_pitch, pp, src->nchan, src->width, src->height );
        imCopyTranspose<u8,u8>( dst->data, dst_pitch, 
                                src->data, src_pitch, shape, 0, 1 ); 
        
      }
      break;
    case FRAME_INTERLEAVED_PLANES:
      if(!dst) return sz;
      src->format(dst);                   // Format metadata is the same so just copy it in
      dst->id = FRAME_INTERLEAVED_LINES;  // Set the format tag properly
      { size_t pp = src->Bpp,
               shape[] = { src->width, src->height, src->nchan },
               dst_pitch[4],
               src_pitch[4];
        Compute_Pitch( dst_pitch, pp, src->width, src->nchan, src->height );
        Compute_Pitch( src_pitch, pp, src->width, src->height, src->nchan  );
        imCopyTranspose<u8,u8>( dst->data, dst_pitch, 
                                src->data, src_pitch, shape, 1, 2 ); 
        
      }
      break;
    default:
      warning("Failed attempt to translate a message.");
      return 0; // no translation
  }
  return sz;
}

/*
 * Frame_With_Interleaved_Planes
 *
 */

Frame_With_Interleaved_Planes::
  Frame_With_Interleaved_Planes(u16 width, u16 height, u8 nchan, Basic_Type_ID type)
                              : width(width),
                                height(height),
                                nchan(nchan),
                                type(type),
                                Bpp( g_type_attributes[type].bytes ),
                                self_size(sizeof(Frame_With_Interleaved_Lines)),
                                id( FRAME_INTERLEAVED_PLANES )
{}

void Frame_With_Interleaved_Planes::
copy_channel( void *dst, size_t rowpitch, size_t ichan )
{ size_t pp = this->Bpp,
         dstw = rowpitch/pp,
         shape[] = {1,
                    MIN( this->width, dstw ),
                    this->height},
         dst_pitch[],
         src_pitch[];
  Compute_Pitch( dst_pitch, pp,        dstw, this->height, 1 );
  Compute_Pitch( src_pitch, pp, this->width, this->height, this->nchan );
  imCopy<u8,u8>(dst,                       dst_pitch,
                src + ichan * src_pitch[1],src_pitch,
                shape);
}

size_t
Frame_With_Interleaved_Planes::
  translate( Message *dst, Message *src )
{ size_t sz = src->size_bytes();

  switch( src->id )
  { case FRAME_INTERLEAVED_PLANES:
      return Message::translate(dst,src);
      break;
    case FRAME_INTERLEAVED_PIXELS:
      if(!dst) return sz;
      src->format(dst);                   // Format metadata is the same so just copy it in
      dst->id = FRAME_INTERLEAVED_PLANES; // Set the format tag properly 
      { size_t pp = src->Bpp,
               shape[] = { src->nchan, src->width, src->height },
               dst_pitch[4],
               src_pitch[4];
        // This ends up apparently transposing width and height.
        // Really, doing this properly requires two transposes.
        Compute_Pitch( dst_pitch, pp, src->height, src->width, src->nchan );
        Compute_Pitch( src_pitch, pp, src->nchan, src->width,  src->height );
        imCopyTranspose<u8,u8>( dst->data, dst_pitch, 
                                src->data, src_pitch, shape, 0, 2 ); 
        
      }
      break;
    case FRAME_INTERLEAVED_LINES:
      if(!dst) return sz;
      src->format(dst);                   // Format metadata is the same so just copy it in
      dst->id = FRAME_INTERLEAVED_PLANES; // Set the format tag properly
      { size_t pp = src->Bpp,
               shape[] = { src->width, src->nchan, src->height },
               dst_pitch[4],
               src_pitch[4];
        Compute_Pitch( dst_pitch, pp, src->width, src->height, src->nchan );
        Compute_Pitch( src_pitch, pp, src->width, src->nchan,  src->height);
        imCopyTranspose<u8,u8>( dst->data, dst_pitch, 
                                src->data, src_pitch, shape, 1, 2 ); 
        
      }
      break;
    default:
      warning("Failed attempt to translate a message.");
      return 0; // no translation
  }
  return sz;
}
