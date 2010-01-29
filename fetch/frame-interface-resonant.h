#pragma once

#include "stdafx.h"
#include "frame.h"
#include "types.h"

#define FRAME_INTERFACE_RESONANT_INTERLEAVED_LINES__METADATA_BYTES  16 //sizeof(t_resonant_frame_metadata);

typedef struct _t_resonant_frame_metadata
{ u16           in_width;
  u16           in_height;
  u8            nchan;
  u8            Bpp;
  f32           aspect;                               // Input (Scan Width) / (Scan Height)
  f32           duty;  
  Basic_Type_ID rtti;
} Resonant_Frame_Metadata;


Basic_Type_ID frame_interface_resonant__default__get_type                 ( Frame_Descriptor* fd);
size_t        frame_interface_resonant__default__get_nchannels            ( Frame_Descriptor* fd);                          // get number of channels
size_t        frame_interface_resonant__default__get_src_nbytes           ( Frame_Descriptor* fd);                          // get size of internal buffer (bytes)
size_t        frame_interface_resonant__default__get_dst_nbytes           ( Frame_Descriptor* fd);                          // get size of destination buffer (single channel) in bytes
void          frame_interface_resonant__default__get_src_dimensions       ( Frame_Descriptor* fd, vector_size_t *vdim );    // returns the dimensions of the source buffer
void          frame_interface_resonant__default__get_dst_dimensions       ( Frame_Descriptor* fd, vector_size_t *vdim );    // returns the dimensions of the destination buffer

void          frame_interface_resonant_interleaved_lines__copy_channel    ( Frame_Descriptor* fd, void *dst, size_t dst_stride, void *src, size_t ichan );   // copies channel by index