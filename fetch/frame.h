// A Frame_Interface is an abstract interface for defining interfaces that interpret frame data.
// Frame data is distinguished from a big bag of bytes by having:
// - channels
// - dimensions
//
// Each interface defines what these mean.  A interface is identified by a 
// unique interger and is an index into a table which is defined in 
// frame.cpp.
//
// This interface is used in streaming data to video and to file.
//

#pragma once
#include "stdafx.h" 

#define FRAME_DESCRIPTOR_MAX_METADATA_BYTES 256

typedef void Frame;

typedef struct _t_frame_descriptor
{ u8                       change_token;        // Used to signal the frame format is a change.  When false the rest of the structure should be ignored.
  u8                       interface_id;  
  u8                       metadata[ FRAME_DESCRIPTOR_MAX_METADATA_BYTES ];
  u32                      metadata_nbytes;
} Frame_Descriptor;

typedef size_t    (*tfp_frame_get_nchannels)  ( Frame_Descriptor* fd);                              // gets channel count
typedef size_t    (*tfp_frame_get_nbytes)     ( Frame_Descriptor* fd);                              // gets channel count
typedef void*     (*tfp_frame_get_channel)    ( Frame_Descriptor* fd, void *src, size_t ichan );    // gets a pointer to channel by index
typedef void      (*tfp_frame_get_dimensions) ( Frame_Descriptor* fd, vector_size_t *vdim);         // returns the dimensions and number of dimensions of the channel

typedef struct _t_frame_interface
{ tfp_frame_get_nchannels  get_nchannels;    // Abstract interface
  tfp_frame_get_nbytes     get_nbytes;
  tfp_frame_get_channel    get_channel;
  tfp_frame_get_dimensions get_dimensions;  
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
// 2. Use Frame_From_Bytes to get access to the data and the frame description.
// 3. Set the descriptor
// - or -
// 1. Use Frame_Alloc
// 2. Use Frame_From_Bytes to get access to the data and the frame description.
// 

size_t            Frame_Get_Size_Bytes ( Frame_Descriptor *desc );
Frame*            Frame_Alloc          ( Frame_Descriptor *desc );
void              Frame_Free           ( void );
void              Frame_From_Bytes     ( Frame *bytes, void **data, Frame_Descriptor **desc );
