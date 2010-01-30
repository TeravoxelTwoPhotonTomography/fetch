// A Frame_Interface is an abstract interface for defining how to interpret frame data.
// Frame data is distinguished from a big bag of bytes by having:
// - channels
// - dimensions
//
// Each interface defines what these mean.  An interface is identified by a 
// unique interger and is an index into a function table that is defined in 
// frame.cpp.
//
// This interface is used in streaming data to video and to file.
//
// This interface is centered around a single operation: "Copy Channel."
// There is a facility for discovering storing source data, and for 
// inquiring about what kind of destination storage is required for the
// copy.
//


//
// TODO/NOTES
// ==========
//
// Refactor to use a union for frame format data
// ---------------------------------------------
// A "frame" is a header describing the format followed by raw data.
// The header can have various formats but is always identified by an
// integer (the interface id) that names the format.  This allows the
// header to by identified.  There are some common elements in the header
// (such as the change_token) that relate to the application and not
// the formating of the image.
//
// When I initially wrote this, it came together in a piecmeal fashion.
// Now it's clear to me that the pattern I want is a union for the
// metadata field rather than a fixed-width buffer.
//
// I think this would simplify the API around using a Frame object.
// 
// A possible disadvantage is that it might complicate compatibility with
// files written with this framework (something not implemented at present).
// Specifically, it might make backwards compatibility break when new 
// frame formats are added.
//

#pragma once
#include "stdafx.h"
#include "types.h"

#define FRAME_DESCRIPTOR_MAX_METADATA_BYTES 256

#define FRAME_INTERFACE_DIGITIZER_INTERLEAVED_PLANES__INTERFACE_ID   0
#define FRAME_INTERFACE_DIGITIZER_INTERLEAVED_LINES__INTERFACE_ID    1
#define FRAME_INTERFACE_RESONANT_INTERLEAVED_LINES__INTERFACE_ID     2

typedef void Frame;

typedef struct _t_frame_descriptor
{ u8                       change_token;        // Used to signal the frame format is a change.  When false the rest of the structure should be ignored.
  u8                       interface_id;
  u32                      metadata_nbytes;
  u8                       metadata[ FRAME_DESCRIPTOR_MAX_METADATA_BYTES ];       // TODO - refactor? - should use a union here.  requires api change
} Frame_Descriptor;

typedef Basic_Type_ID (*tfp_frame_get_type)                    ( Frame_Descriptor* fd);                              // get pixel type
typedef size_t        (*tfp_frame_get_nchannels)               ( Frame_Descriptor* fd);                              // gets channel count
typedef size_t        (*tfp_frame_get_source_nbytes)           ( Frame_Descriptor* fd);                              // gets size of internal buffer in bytes (covers all channels)
typedef size_t        (*tfp_frame_get_destination_nbytes)      ( Frame_Descriptor* fd);                              // gets size needed for destination buffer (one channel)
typedef void          (*tfp_frame_copy_channel)                ( Frame_Descriptor* fd, void *dst, size_t dst_stride, void *src, size_t ichan );    // copies channel data to dst
typedef void          (*tfp_frame_get_source_dimensions)       ( Frame_Descriptor* fd, vector_size_t *vdim);         // returns the dimensions and number of dimensions of the channel
typedef void          (*tfp_frame_get_destination_dimensions)  ( Frame_Descriptor* fd, vector_size_t *vdim);         // returns the dimensions and number of dimensions of the channel

typedef struct _t_frame_interface
{ tfp_frame_get_type                   get_type;                  // Abstract interface
  tfp_frame_get_nchannels              get_nchannels;
  tfp_frame_get_source_nbytes          get_source_nbytes;
  tfp_frame_get_destination_nbytes     get_destination_nbytes;
  tfp_frame_copy_channel               copy_channel;
  tfp_frame_get_source_dimensions      get_source_dimensions;
  tfp_frame_get_destination_dimensions get_destination_dimensions;
} Frame_Interface;

//
// Frame Descriptor
//

inline Frame_Interface  *Frame_Descriptor_Get_Interface ( Frame_Descriptor *self );
       void              Frame_Descriptor_Change        ( Frame_Descriptor *self, u8 interface_id, void *metadata, size_t nbytes );

// TODO: Finish file i/o interface
void              Frame_Descriptor_To_File              ( FILE *fp, Frame_Descriptor *self );
u8                Frame_Descriptor_From_File_Read_Next  ( FILE *fp, Frame_Descriptor *self, size_t *repeat_count );

//
// Frame
//
// To create a frame:
//
// 1. allocate a buffer of size Frame_Get_Size_Bytes with malloc
// 2. Use Frame_Set to get access to the data and the frame description.
// 3. Set the descriptor
// - or -
// 1. Use Frame_Alloc
// 2. Use Frame_Set to get access to the data and the frame description.
// 

size_t            Frame_Get_Size_Bytes ( Frame_Descriptor *desc );                             // Returns size of frame in bytes (descriptor + internal buffer)
Frame*            Frame_Alloc          ( Frame_Descriptor *desc );
void              Frame_Free           ( void );
void              Frame_Set            ( Frame *bytes, void **data, Frame_Descriptor **desc );
