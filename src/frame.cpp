#include "common.h"
#include "config.h"
#include "frame.h"
#include "util/util-file.h"
#include "util/util-image.h"

#include "util/util-mylib.h"
namespace mylib {
#include <array.h>
#include <image.h>
}

#define FRAME_WARN_ON_DUMP

namespace fetch
{
/*
 * MESSAGE
 */

void
Message::to_file(FILE *fp)                                     // Writing to a file/byte stream                                           
{ size_t sz  = this->size_bytes(),                             // -----------------------------                                           
         off = (u8*)this->data - (u8*)this;                    // 0. Compute the size of the formating data
  Guarded_Assert( 1 == fwrite( &sz,  sizeof(size_t), 1, fp )); // 1. The size (in bytes) is written to the stream.
  Guarded_Assert( 1 == fwrite( &off, sizeof(size_t), 1, fp )); // 2. WriteRaw out the size of the formating data.
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
  i64  bookmark = w32file::getpos(hfile);
  Guarded_Assert_WinErr(    ReadFile( hfile,      &sz, sizeof(size_t), &bytes_read, NULL ));  

  if( !workspace ||  size_workspace < sz )
  { w32file::setpos(hfile,bookmark,FILE_BEGIN);
    return sz+2*sizeof(size_t);               // failure.  Indicates the step size to the next message.
  }

  Guarded_Assert_WinErr(    ReadFile( hfile,      &off, sizeof(size_t), &bytes_read, NULL ));  
  Guarded_Assert_WinErr(    ReadFile( hfile, workspace,             sz, &bytes_read, NULL ));  
  workspace->data = (u8*)workspace + off;
  return 0; // Success.
}

size_t
Message::copy_data(Message *dst, Message *src)
{ size_t srcbytes = src->size_bytes() - src->self_size;
  size_t dstbytes = dst->size_bytes() - dst->self_size;
  Guarded_Assert(srcbytes==dstbytes);
  memcpy(dst->data,src->data,srcbytes);
  return srcbytes;
}

size_t
Message::translate( Message *dst, Message *src )
{ src->format(dst);
  return Message::copy_data(dst,src);
}

void
  Message::cast(void)
{ switch( this->id )
  { case FRAME_INTERLEAVED_PLANES: __cast<Frame_With_Interleaved_Planes>(); break;
    case FRAME_INTERLEAVED_LINES:  __cast<Frame_With_Interleaved_Lines>();  break;
    case FRAME_INTERLEAVED_PIXELS: __cast<Frame_With_Interleaved_Pixels>(); break;
    default:
      break; //do nothing          
  }
}

/*
 * FrmFmt
 */

FrmFmt::FrmFmt()
      : Message(FORMAT_INVALID, sizeof(Frame)),
        width(0),
        height(0),
        nchan(0),
        rtti(id_unspecified),
        Bpp(0)
{}

FrmFmt::FrmFmt(u32 width, u32 height, u8 nchan, Basic_Type_ID type)
      : Message(FORMAT_INVALID, sizeof(Frame)),
        width(width),
        height(height),
        nchan(nchan),
        rtti(type),
        Bpp( g_type_attributes[type].bytes )
{}

FrmFmt::FrmFmt(u32 width, u32 height, u8 nchan, Basic_Type_ID type, MessageFormatID id, size_t self_size)
      : Message(id, self_size),
        width(width),
        height(height),
        nchan(nchan),
        rtti(type),
        Bpp( g_type_attributes[type].bytes )
{}

unsigned int
FrmFmt::is_equivalent( FrmFmt *ref )
{ if( (ref->id     == this->id      ) &&
      (ref->width  == this->width   ) &&
      (ref->height == this->height  ) &&
      (ref->nchan  == this->nchan   ) &&
      (ref->rtti   == this->rtti    ) ) return 1;
  return 0;
}

LPCRITICAL_SECTION g_p_frmfmtdump_critical_section = NULL;
    
static LPCRITICAL_SECTION _get_frmfmtdump_critical_section(void)
{ static CRITICAL_SECTION gcs;
  if(!g_p_frmfmtdump_critical_section)
  { InitializeCriticalSectionAndSpinCount( &gcs, 0x80000400 ); // don't assert - Release builds will optimize this out.
    g_p_frmfmtdump_critical_section = &gcs;
  }
  return g_p_frmfmtdump_critical_section;
}    

#include <stdarg.h>
void
FrmFmt::dump( const char *fmt,...)
{ EnterCriticalSection( _get_frmfmtdump_critical_section() );
  {
    size_t  n = this->width * 
                this->height*
                this->nchan;
    size_t len;    
    va_list     argList;
    static      vector_char vbuf = VECTOR_EMPTY;

    // render formated string
    va_start( argList, fmt );
      len = _vscprintf(fmt,argList) + 1;
      vector_char_request( &vbuf, len );
      memset  ( vbuf.contents, 0, len );
  #pragma warning( push )
  #pragma warning( disable:4996 )
  #pragma warning( disable:4995 )
      vsprintf( vbuf.contents, fmt, argList);
  #pragma warning( pop )
    va_end( argList );  
    
#ifdef FRAME_WARN_ON_DUMP
    warning("FrmFmt::Dump() - Writing %s\r\n", vbuf.contents);
#endif

    // Write
    FILE *fp = fopen(vbuf.contents,"wb");
    fwrite(this->data, this->Bpp, n, fp );
    fclose(fp);
  }
  LeaveCriticalSection( _get_frmfmtdump_critical_section() );
}

size_t
FrmFmt::size_bytes(void)
{ return this->self_size + this->width * 
                           this->height*
                           this->nchan *
                           this->Bpp;
}

void
FrmFmt::format(Message* unformatted)
{ memcpy( unformatted, this, this->self_size );           // Copy the format header (also copies vtable addr)
  unformatted->data = (u8*)unformatted + this->self_size; // Set the data section
}


void
Frame::totif( const char *fmt,...)
{ EnterCriticalSection( _get_frmfmtdump_critical_section() );
  {
    size_t  n = this->width * 
                this->height*
                this->nchan;
    size_t len;    
    va_list     argList;
    static      vector_char vbuf = VECTOR_EMPTY;

    // render formated string
    va_start( argList, fmt );
      len = _vscprintf(fmt,argList) + 1;
      vector_char_request( &vbuf, len );
      memset  ( vbuf.contents, 0, len );
  #pragma warning( push )
  #pragma warning( disable:4996 )
  #pragma warning( disable:4995 )
      vsprintf( vbuf.contents, fmt, argList);
  #pragma warning( pop )
    va_end( argList );  
    
#ifdef FRAME_WARN_ON_DUMP
    warning("Frame::totif() - Writing %s\r\n", vbuf.contents);
#endif

    // Write
    { mylib::Array     a;
      mylib::Dimn_Type dims[3];
      mylib::castFetchFrameToDummyArray(&a,this,dims);
      mylib::Write_Image(vbuf.contents,&a,mylib::DONT_PRESS);
    }
  }
  LeaveCriticalSection( _get_frmfmtdump_critical_section() );
}

/*
 * Frame_With_Interleaved_Pixels
 *
 */

Frame_With_Interleaved_Pixels::
  Frame_With_Interleaved_Pixels(u32 width, u32 height, u8 nchan, Basic_Type_ID type)
                              : Frame(width,
                                      height,
                                      nchan,
                                      type,
                                      FRAME_INTERLEAVED_PIXELS,
                                      sizeof(Frame_With_Interleaved_Pixels))
{}

void 
Frame_With_Interleaved_Pixels::
  copy_channel( void *dst, size_t rowpitch, size_t ichan )
{ size_t pp = this->Bpp,
         dstw = rowpitch/pp,
         shape[] = {this->height,
                    MIN( this->width, dstw ),
                    1},
         dst_pitch[4], src_pitch[4];
  Compute_Pitch( dst_pitch, this->height,        dstw,           1, pp );
  Compute_Pitch( src_pitch, this->height, this->width, this->nchan, pp );
  imCopy<u8,u8>((u8*)dst                               , dst_pitch,
                (u8*)this->data + ichan * src_pitch[3] , src_pitch,
                shape);
}

size_t
Frame_With_Interleaved_Pixels::
  translate( Message *mdst, Message *msrc )
{ size_t sz = msrc->size_bytes();
  Frame *dst = dynamic_cast<Frame*>( mdst ),
        *src = dynamic_cast<Frame*>( msrc );

  switch( src->id )
  { case FRAME_INTERLEAVED_PIXELS:
      return Message::translate(dst,src);
      break;
    case FRAME_INTERLEAVED_LINES:
      if(!dst) return sz;
      src->format(dst);                   // Format metadata is the same so just copy it in
      dst->id = FRAME_INTERLEAVED_PIXELS; // Set the format tag properly
      { size_t pp = src->Bpp,
               shape[] = {  src->height, src->nchan, pp*src->width},
               dst_pitch[4],
               src_pitch[4];
        Compute_Pitch( dst_pitch, src->height, src->width, src->nchan, pp );
        Compute_Pitch( src_pitch, src->height, src->nchan, src->width, pp );
        imCopyTranspose<u8,u8>( (u8*) dst->data, dst_pitch, 
                                (u8*) src->data, src_pitch, shape, 1, 2 ); 
        
      }
      break;
    case FRAME_INTERLEAVED_PLANES:
      if(!dst) return sz;
      src->format(dst);                   // Format metadata is the same so just copy it in
      dst->id = FRAME_INTERLEAVED_PIXELS; // Set the format tag properly
      { size_t pp = src->Bpp,
               shape[] = {  src->width, src->height, pp*src->nchan},
               dst_pitch[4],
               src_pitch[4];
        // This ends up apparently transposing width and height.
        // Really, doing this properly requires two transposes.
        Compute_Pitch( dst_pitch, src->width, src->height, src->nchan, pp );
        Compute_Pitch( src_pitch, src->nchan, src->height, src->width, pp  );
        imCopyTranspose<u8,u8>( (u8*) dst->data, dst_pitch, 
                                (u8*) src->data, src_pitch, shape, 0, 2 ); 
        
      }
      break;
    default:
      warning("Failed attempt to translate a message.");
      return 0; // no translation
  }
  return sz;
}

void
Frame_With_Interleaved_Pixels::compute_pitches( size_t pitch[4] )
{ Compute_Pitch( pitch, this->height, this->width, this->nchan, this->Bpp );
}

void
Frame_With_Interleaved_Pixels::get_shape( size_t n[3] )
{ n[0] = this->height;
  n[1] = this->width;
  n[2] = this->nchan;
}

/*
 * Frame_With_Interleaved_Planes
 *
 */

Frame_With_Interleaved_Lines::
  Frame_With_Interleaved_Lines(u32 width, u32 height, u8 nchan, Basic_Type_ID type)
                             : Frame(width,
                                     height,
                                     nchan,
                                     type,
                                     FRAME_INTERLEAVED_LINES,
                                     sizeof(Frame_With_Interleaved_Lines))
{}

void Frame_With_Interleaved_Lines::
copy_channel( void *dst, size_t rowpitch, size_t ichan )
{ size_t pp = this->Bpp,
         dstw = rowpitch/pp,
         shape[] = {this->height,
                    1,
                    pp*MIN( this->width, dstw )},
         dst_pitch[4],
         src_pitch[4];
  Compute_Pitch( dst_pitch, this->height,           1,        dstw, pp );
  Compute_Pitch( src_pitch, this->height, this->nchan, this->width, pp );
  imCopy<u8,u8>((u8*) dst,                              dst_pitch,
                (u8*) this->data + ichan * src_pitch[2],src_pitch,
                shape);
}

size_t
Frame_With_Interleaved_Lines::
  translate( Message *mdst, Message *msrc )
{ size_t sz = msrc->size_bytes();
  Frame *dst = dynamic_cast<Frame*>( mdst ),
        *src = dynamic_cast<Frame*>( msrc );

  switch( src->id )
  { case FRAME_INTERLEAVED_LINES:
      return Message::translate(dst,src);
      break;
    case FRAME_INTERLEAVED_PIXELS:
      if(!dst) return sz;
      src->format(dst);                   // Format metadata is the same so just copy it in
      dst->id = FRAME_INTERLEAVED_LINES;  // Set the format tag properly 
      { size_t pp = src->Bpp,
               shape[] = { src->height, src->width, pp*src->nchan },
               dst_pitch[4],
               src_pitch[4];
        Compute_Pitch( dst_pitch, src->height, src->nchan, src->width, pp );
        Compute_Pitch( src_pitch, src->height, src->width, src->nchan, pp );
        imCopyTranspose<u8,u8>( (u8*) dst->data, dst_pitch, 
                                (u8*) src->data, src_pitch, shape, 1, 2 ); 
        
      }
      break;
    case FRAME_INTERLEAVED_PLANES:
      if(!dst) return sz;
      src->format(dst);                   // Format metadata is the same so just copy it in
      dst->id = FRAME_INTERLEAVED_LINES;  // Set the format tag properly
      { size_t pp = src->Bpp,
               shape[] = { src->nchan, src->height, pp*src->width },
               dst_pitch[4],
               src_pitch[4];
        Compute_Pitch( dst_pitch, src->height,  src->nchan, src->width, pp );
        Compute_Pitch( src_pitch,  src->nchan, src->height, src->width, pp  );
        imCopyTranspose<u8,u8>( (u8*) dst->data, dst_pitch, 
                                (u8*) src->data, src_pitch, shape, 0, 1 ); 
        
      }
      break;
    default:
      warning("Failed attempt to translate a message.");
      return 0; // no translation
  }
  return sz;
}

void
Frame_With_Interleaved_Lines::compute_pitches( size_t pitch[4] )
{ Compute_Pitch( pitch, this->height, this->nchan, this->width, this->Bpp );
}

void
Frame_With_Interleaved_Lines::get_shape( size_t n[3] )
{ n[0] = this->height;
  n[1] = this->nchan;
  n[2] = this->width;
}

/*
 * Frame_With_Interleaved_Planes
 *
 */

Frame_With_Interleaved_Planes::
  Frame_With_Interleaved_Planes(u32 width, u32 height, u8 nchan, Basic_Type_ID type)
                              : Frame(width,
                                      height,
                                      nchan,
                                      type,
                                      FRAME_INTERLEAVED_PLANES,
                                      sizeof(Frame_With_Interleaved_Planes))
{}

void Frame_With_Interleaved_Planes::
copy_channel( void *dst, size_t rowpitch, size_t ichan )
{ size_t pp = this->Bpp,
         dstw = rowpitch/pp,
         shape[] = {1,
                    pp*MIN( this->width, dstw ),
                    this->height},
         dst_pitch[4],
         src_pitch[4];
  Compute_Pitch( dst_pitch,           1, this->height,        dstw*pp, 1 );
  Compute_Pitch( src_pitch, this->nchan, this->height, this->width*pp, 1 );
  imCopy<u8,u8>((u8*) dst,                              dst_pitch,
                (u8*) this->data + ichan * src_pitch[1],src_pitch,
                shape);
}

size_t
Frame_With_Interleaved_Planes::
  translate( Message *mdst, Message *msrc )
{ size_t sz = msrc->size_bytes();
  Frame *dst = dynamic_cast<Frame*>( mdst ),
        *src = dynamic_cast<Frame*>( msrc );  

  switch( src->id )
  { case FRAME_INTERLEAVED_PLANES:
      return Message::translate(dst,src);
      break;
    case FRAME_INTERLEAVED_PIXELS:
      if(!dst) return sz;
      src->format(dst);                   // Format metadata is the same so just copy it in
      dst->id = FRAME_INTERLEAVED_PLANES; // Set the format tag properly 
      { size_t pp = src->Bpp,
               shape[] = { src->height , src->width, pp*src->nchan},
               dst_pitch[4],
               src_pitch[4];
        // This ends up apparently transposing width and height.
        // Really, doing this properly requires two transposes.
        Compute_Pitch( dst_pitch,  src->nchan, src->width, src->height, pp );
        Compute_Pitch( src_pitch, src->height, src->width,  src->nchan, pp );
        imCopyTranspose<u8,u8>( (u8*) dst->data, dst_pitch, 
                                (u8*) src->data, src_pitch, shape, 0, 2 ); 
        
      }
      break;
    case FRAME_INTERLEAVED_LINES:
      if(!dst) return sz;
      src->format(dst);                   // Format metadata is the same so just copy it in
      dst->id = FRAME_INTERLEAVED_PLANES; // Set the format tag properly
      { size_t pp = src->Bpp,
               shape[] = { src->height, src->nchan, pp*src->width },
               dst_pitch[4],
               src_pitch[4];
        Compute_Pitch( dst_pitch,  src->nchan, src->height, src->width, pp );
        Compute_Pitch( src_pitch, src->height,  src->nchan, src->width, pp );
        imCopyTranspose<u8,u8>( (u8*) dst->data, dst_pitch, 
                                (u8*) src->data, src_pitch, shape, 0, 1); 
        
      }
      break;
    default:
      warning("Failed attempt to translate a message.");
      return 0; // no translation
  }
  { // cast
    Frame_With_Interleaved_Planes ref(dst->width,dst->height,dst->nchan,dst->rtti);
    ref.format(dst);
  }
  return sz;
}

void
Frame_With_Interleaved_Planes::compute_pitches( size_t pitch[4] )
{ Compute_Pitch( pitch, this->nchan, this->height, this->width, this->Bpp );
}

void
Frame_With_Interleaved_Planes::get_shape( size_t n[3] )
{ n[0] = this->nchan;
  n[1] = this->height;
  n[2] = this->width;
}

} //end namespace fetch