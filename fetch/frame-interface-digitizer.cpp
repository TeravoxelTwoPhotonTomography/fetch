#pragma once
#include "stdafx.h"
#include "frame.h"
#include "frame-interface-digitizer.h"

#define DEBUG_FRAME_INTERFACE_DIGITIZER_INTERLEAVED_LINES
#if 0
#define DEBUG_FRAME_INTERFACE_DIGITIZER_INTERLEAVED_PLANES
#endif

//
// Defaults
// These functions will probably be resused for different frame formats
//

size_t
frame_interface_digitizer__default__get_nchannels (Frame_Descriptor* fd)
{ Digitizer_Frame_Metadata *meta = (Digitizer_Frame_Metadata*) fd->metadata;

  return meta->nchan;
}

size_t
frame_interface_digitizer__default__get_nbytes (Frame_Descriptor* fd)
{ //DEBUG_FRAME_INTERFACE_DIGITIZER_INTERLEAVED_PLANES__CHECK_DESCRIPTOR;
  Digitizer_Frame_Metadata *meta = (Digitizer_Frame_Metadata*) fd->metadata;
  return meta->width * meta->height * meta->nchan * meta->Bpp;
}

void
frame_interface_digitizer__default__copy_channel( Frame_Descriptor* fd, void *dst, size_t dst_stride, void *src, size_t ichan )
{ //DEBUG_FRAME_INTERFACE_DIGITIZER_INTERLEAVED_PLANES__CHECK_DESCRIPTOR;
  Digitizer_Frame_Metadata        *meta = (Digitizer_Frame_Metadata*) fd->metadata;  
  size_t                    chan_stride = meta->width * meta->height * meta->Bpp;
  
  Guarded_Assert(0); // TODO: FIX: adapt for copies to desitinations with different line strides.  Do fast copy when strides are the same.
  
  memcpy( dst, (u8*)src + chan_stride * ichan , chan_stride );
  return;
}

void
frame_interface_digitizer__default__get_dimensions ( Frame_Descriptor* fd, vector_size_t *vdim )
{ //DEBUG_FRAME_INTERFACE_DIGITIZER_INTERLEAVED_PLANES__CHECK_DESCRIPTOR;
  Digitizer_Frame_Metadata        *meta = (Digitizer_Frame_Metadata*) fd->metadata;
  vector_size_t_request( vdim, 2 );
  vdim->count       = 2;
  vdim->contents[0] = meta->width;
  vdim->contents[1] = meta->height;
}

//
// Interleaved planes
// Each channel is in it's own contiguous memory block
//
#ifdef DEBUG_FRAME_INTERFACE_DIGITIZER_INTERLEAVED_PLANES
#define DEBUG_FRAME_INTERFACE_DIGITIZER_INTERLEAVED_PLANES__CHECK_DESCRIPTOR\
  { Guarded_Assert( fd->interface_id    == FRAME_INTERFACE_DIGITIZER_INTERLEAVED_PLANES__INTERFACE_ID );\
    Guarded_Assert( fd->metadata_nbytes == FRAME_INTERFACE_DIGITIZER_INTERLEAVED_PLANES__METADATA_BYTES;\
  }
#else
  #define DEBUG_FRAME_INTERFACE_DIGITIZER_INTERLEAVED_PLANES__CHECK_DESCRIPTOR
#endif

void
frame_interface_digitizer_interleaved_planes__copy_channel( Frame_Descriptor* fd, void *dst, size_t dst_stride, void *src, size_t ichan )
{ DEBUG_FRAME_INTERFACE_DIGITIZER_INTERLEAVED_PLANES__CHECK_DESCRIPTOR;
  Digitizer_Frame_Metadata        *meta = (Digitizer_Frame_Metadata*) fd->metadata;  
  size_t                    chan_stride = meta->width * meta->height * meta->Bpp;
  
  Guarded_Assert(0); // TODO: FIX: adapt for copies to desitinations with different line strides.  Do fast copy when strides are the same.
  
  memcpy( dst, (u8*)src + chan_stride * ichan , chan_stride );
  return;
}

//
// Interleaved lines
// Within a line, each channel is contiguous in memory.
//

#ifdef DEBUG_FRAME_INTERFACE_DIGITIZER_INTERLEAVED_LINES
#define DEBUG_FRAME_INTERFACE_DIGITIZER_INTERLEAVED_LINES__CHECK_DESCRIPTOR\
  { Guarded_Assert( fd->interface_id    == FRAME_INTERFACE_DIGITIZER_INTERLEAVED_LINES__INTERFACE_ID );\
    Guarded_Assert( fd->metadata_nbytes == FRAME_INTERFACE_DIGITIZER_INTERLEAVED_LINES__METADATA_BYTES );\
  }
#else
  #define DEBUG_FRAME_INTERFACE_DIGITIZER_INTERLEAVED_LINES__CHECK_DESCRIPTOR
#endif

void
frame_interface_digitizer_interleaved_lines__copy_channel( Frame_Descriptor* fd, void *dst, size_t dst_stride, void *src, size_t ichan )
{ DEBUG_FRAME_INTERFACE_DIGITIZER_INTERLEAVED_LINES__CHECK_DESCRIPTOR
  Digitizer_Frame_Metadata        *meta = (Digitizer_Frame_Metadata*) fd->metadata;  
  size_t                         nlines = meta->height,
                            line_nbytes = meta->width * meta->Bpp,
                           plane_nbytes = nlines * line_nbytes,
                                 stride = line_nbytes * meta->nchan;
#ifdef DEBUG_FRAME_INTERFACE_DIGITIZER_INTERLEAVED_LINES
  { FILE *fp = fopen("source.raw","wb");    
    fwrite( src, 1, meta->width * meta->height * meta->nchan * meta->Bpp, fp );
    fclose(fp); 
  }
#endif
  u8 *src_cur = (u8*)src + stride * nlines + ichan * line_nbytes,
     *src_beg = (u8*)src + ichan * line_nbytes,
     *dst_cur = (u8*)dst + dst_stride * nlines;
  while( src_cur > src_beg )
  { src_cur -= stride;
    dst_cur -= dst_stride;
    memcpy( dst_cur, src_cur, line_nbytes );
  }
#ifdef DEBUG_FRAME_INTERFACE_DIGITIZER_INTERLEAVED_LINES
  { FILE *fp = fopen("dest.raw","wb");    
    fwrite( dst, 1, dst_stride * meta->height, fp );
    fclose(fp); 
  }
#endif  
  return;
}