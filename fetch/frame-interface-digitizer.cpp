#pragma once
#include "stdafx.h"
#include "frame.h"
#include "frame-interface-digitizer.h"

#if 0
#define DEBUG_FRAME_INTERFACE_DIGITIZER_INTERLEAVED_PLANES
#endif

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

size_t
frame_interface_digitizer_interleaved_planes__get_nchannels (Frame_Descriptor* fd)
{ DEBUG_FRAME_INTERFACE_DIGITIZER_INTERLEAVED_PLANES__CHECK_DESCRIPTOR;
  Digitizer_Frame_Metadata *meta = (Digitizer_Frame_Metadata*) fd->metadata;
  return meta->nchan;
}

size_t
frame_interface_digitizer_interleaved_planes__get_nbytes (Frame_Descriptor* fd)
{ DEBUG_FRAME_INTERFACE_DIGITIZER_INTERLEAVED_PLANES__CHECK_DESCRIPTOR;
  Digitizer_Frame_Metadata *meta = (Digitizer_Frame_Metadata*) fd->metadata;
  return meta->width * meta->height * meta->Bpp;
}

void*
frame_interface_digitizer_interleaved_planes__get_channel( Frame_Descriptor* fd, void *data, size_t ichan )
{ DEBUG_FRAME_INTERFACE_DIGITIZER_INTERLEAVED_PLANES__CHECK_DESCRIPTOR;
  Digitizer_Frame_Metadata        *meta = (Digitizer_Frame_Metadata*) fd->metadata;  
  size_t                    chan_stride = meta->width * meta->height * meta->Bpp;
  return (u8*)data + chan_stride * ichan;  
}

void
frame_interface_digitizer_interleaved_planes__get_dimensions ( Frame_Descriptor* fd, vector_size_t *vdim )
{ DEBUG_FRAME_INTERFACE_DIGITIZER_INTERLEAVED_PLANES__CHECK_DESCRIPTOR;
  Digitizer_Frame_Metadata        *meta = (Digitizer_Frame_Metadata*) fd->metadata;
  vector_size_t_request( vdim, 2 );
  vdim->count       = 2;
  vdim->contents[0] = meta->width;
  vdim->contents[1] = meta->height;
}

//
// Interleaved lines
// Within a line, each channel is contiguous in memory.
//

//
// Interleaved planes
// Each channel is in it's own contiguous memory block
//
#ifdef DEBUG_FRAME_INTERFACE_DIGITIZER_INTERLEAVED_LINES
#define DEBUG_FRAME_INTERFACE_DIGITIZER_INTERLEAVED_LINES__CHECK_DESCRIPTOR\
  { Guarded_Assert( fd->interface_id    == FRAME_INTERFACE_DIGITIZER_INTERLEAVED_LINES__INTERFACE_ID );\
    Guarded_Assert( fd->metadata_nbytes == FRAME_INTERFACE_DIGITIZER_INTERLEAVED_LINES__METADATA_BYTES;\
  }
#else
  #define DEBUG_FRAME_INTERFACE_DIGITIZER_INTERLEAVED_LINES__CHECK_DESCRIPTOR
#endif

size_t
frame_interface_digitizer_interleaved_lines__get_nchannels (Frame_Descriptor* fd)
{ DEBUG_FRAME_INTERFACE_DIGITIZER_INTERLEAVED_LINES__CHECK_DESCRIPTOR;
  Digitizer_Frame_Metadata *meta = (Digitizer_Frame_Metadata*) fd->metadata;
  return meta->nchan;
}

size_t
frame_interface_digitizer_interleaved_lines__get_nbytes (Frame_Descriptor* fd)
{ DEBUG_FRAME_INTERFACE_DIGITIZER_INTERLEAVED_LINES__CHECK_DESCRIPTOR;
  Digitizer_Frame_Metadata *meta = (Digitizer_Frame_Metadata*) fd->metadata;
  return meta->width * meta->height * meta->Bpp;
}

void*
frame_interface_digitizer_interleaved_lines__get_channel( Frame_Descriptor* fd, void *data, size_t ichan )
{ DEBUG_FRAME_INTERFACE_DIGITIZER_INTERLEAVED_LINES__CHECK_DESCRIPTOR;
  Digitizer_Frame_Metadata        *meta = (Digitizer_Frame_Metadata*) fd->metadata;  
  size_t                         nbytes = meta->width * meta->height * meta->Bpp,
                                 stride = meta->width * meta->nchan  * meta->Bpp;
  int                            nlines = meta->height;
  static vector_u8 *buf = 0;
  if(!buf)
    buf = vector_u8_alloc( nbytes );
  vector_u8_request( buf, nbytes - 1 /*max index*/);
  { // WIP
    // TODO
  }
  return (u8*)data + chan_stride * ichan;
}

void
frame_interface_digitizer_interleaved_lines__get_dimensions ( Frame_Descriptor* fd, vector_size_t *vdim )
{ DEBUG_FRAME_INTERFACE_DIGITIZER_INTERLEAVED_LINES__CHECK_DESCRIPTOR;
  Digitizer_Frame_Metadata        *meta = (Digitizer_Frame_Metadata*) fd->metadata;
  vector_size_t_request( vdim, 2 );
  vdim->count       = 2;
  vdim->contents[0] = meta->width;
  vdim->contents[1] = meta->height;
}