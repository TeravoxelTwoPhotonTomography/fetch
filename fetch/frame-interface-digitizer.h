#pragma once

#include "stdafx.h"
#include "frame.h"

#define FRAME_INTEFACE_DIGITIZER__INTERFACE_ID   0
#define FRAME_INTEFACE_DIGITIZER__METADATA_BYTES 6 // sizeof(t_digitizer_frame_description);

typedef struct _t_digitizer_frame_metadata
{ u16 width;
  u16 height;
  u8  nchan;
  u8  Bpp;
} Digitizer_Frame_Metadata;

size_t    frame_interface_digitizer_get_nchannels  ( Frame_Descriptor* fd);                              // gets channel count
size_t    frame_interface_digitizer_get_nbytes     ( Frame_Descriptor* fd);                              // gets bytes per channel
void*     frame_interface_digitizer_get_channel    ( Frame_Descriptor* fd, void *data, size_t ichan );   // gets a pointer to channel by index
void      frame_interface_digitizer_get_dimensions ( Frame_Descriptor* fd, vector_size_t *vdim );        // returns the dimensions and number of dimensions of the channel