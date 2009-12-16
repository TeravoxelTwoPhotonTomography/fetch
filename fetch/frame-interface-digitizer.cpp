#pragma once
#include "stdafx.h"
#include "frame.h"
#include "frame-interface-digitizer.h"

#if 0
#define DEBUG_FRAME_INTERFACE_DIGITIZER
#endif

#ifdef DEBUG_FRAME_INTERFACE_DIGITIZER
#define DEBUG_FRAME_INTERFACE_DIGITIZER__CHECK_DESCRIPTOR\
  { Guarded_Assert( fd->interface_id    == FRAME_INTEFACE_DIGITIZER__INTERFACE_ID );\
    Guarded_Assert( fd->metadata_nbytes == FRAME_INTEFACE_DIGITIZER__METADATA_BYTES;\
  }
#else
  #define DEBUG_FRAME_INTERFACE_DIGITIZER__CHECK_DESCRIPTOR
#endif

size_t
frame_interface_digitizer_get_nchannels (Frame_Descriptor* fd)
{ DEBUG_FRAME_INTERFACE_DIGITIZER__CHECK_DESCRIPTOR;
  Digitizer_Frame_Metadata *meta = (Digitizer_Frame_Metadata*) fd->metadata;
  return meta->nchan;
}

size_t
frame_interface_digitizer_get_nbytes (Frame_Descriptor* fd)
{ DEBUG_FRAME_INTERFACE_DIGITIZER__CHECK_DESCRIPTOR;
  Digitizer_Frame_Metadata *meta = (Digitizer_Frame_Metadata*) fd->metadata;
  return meta->width * meta->height * meta->Bpp;
}

void*
frame_interface_digitizer_get_channel( Frame_Descriptor* fd, void *data, size_t ichan )
{ DEBUG_FRAME_INTERFACE_DIGITIZER__CHECK_DESCRIPTOR;
  Digitizer_Frame_Metadata        *meta = (Digitizer_Frame_Metadata*) fd->metadata;  
  size_t                    chan_stride = meta->width * meta->height * meta->Bpp;
  return (u8*)data + chan_stride * ichan;  
}

void
frame_interface_digitizer_get_dimensions ( Frame_Descriptor* fd, vector_size_t *vdim )
{ DEBUG_FRAME_INTERFACE_DIGITIZER__CHECK_DESCRIPTOR;
  Digitizer_Frame_Metadata        *meta = (Digitizer_Frame_Metadata*) fd->metadata;
  vector_size_t_request( vdim, 2 );
  vdim->count       = 2;
  vdim->contents[0] = meta->width;
  vdim->contents[1] = meta->height;
}