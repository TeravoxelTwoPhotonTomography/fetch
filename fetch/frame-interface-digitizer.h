#pragma once

#include "stdafx.h"
#include "frame.h"

#define FRAME_INTERFACE_DIGITIZER_INTERLEAVED_PLANES__INTERFACE_ID   0
#define FRAME_INTERFACE_DIGITIZER_INTERLEAVED_PLANES__INTERFACE_ID   1
#define FRAME_INTERFACE_DIGITIZER_INTERLEAVED_PLANES__METADATA_BYTES 6 // sizeof(t_digitizer_frame_description);

typedef struct _t_digitizer_frame_metadata
{ u16 width;
  u16 height;
  u8  nchan;
  u8  Bpp;
} Digitizer_Frame_Metadata;

size_t    frame_interface_digitizer_interleaved_planes__get_nchannels  ( Frame_Descriptor* fd);                              // gets channel count
size_t    frame_interface_digitizer_interleaved_planes__get_nbytes     ( Frame_Descriptor* fd);                              // gets bytes per channel
void*     frame_interface_digitizer_interleaved_planes__get_channel    ( Frame_Descriptor* fd, void *data, size_t ichan );   // gets a pointer to channel by index
void      frame_interface_digitizer_interleaved_planes__get_dimensions ( Frame_Descriptor* fd, vector_size_t *vdim );        // returns the dimensions and number of dimensions of the channel

size_t    frame_interface_digitizer_interleaved_lines__get_nchannels   ( Frame_Descriptor* fd);                              // gets channel count
size_t    frame_interface_digitizer_interleaved_lines__get_nbytes      ( Frame_Descriptor* fd);                              // gets bytes per channel
void*     frame_interface_digitizer_interleaved_lines__get_channel     ( Frame_Descriptor* fd, void *data, size_t ichan );   // gets a pointer to channel by index
void      frame_interface_digitizer_interleaved_lines__get_dimensions  ( Frame_Descriptor* fd, vector_size_t *vdim );        // returns the dimensions and number of dimensions of the channel