#include "stdafx.h"
#include "frame.h"

#define DEBUG_FRAME_DESCRIPTOR_TO_FILE
#define DEBUG_FRAME_DESCRIPTOR_GET_INTERFACE

//----------------------------------------------------------------------------
// PROTOCOL TABLE
//----------------------------------------------------------------------------

#include "frame-interface-digitizer.h"
#include "frame-interface-resonant.h"

Frame_Interface g_interfaces[] = {
  { frame_interface_digitizer__default__get_nchannels,   // 0 - frame-interface-digitizer-interleaved-planes
    frame_interface_digitizer__default__get_nbytes,
    frame_interface_digitizer__default__get_nbytes,
    frame_interface_digitizer_interleaved_planes__copy_channel,
    frame_interface_digitizer__default__get_dimensions,
    frame_interface_digitizer__default__get_dimensions    
  },  
  { frame_interface_digitizer__default__get_nchannels,   // 1 - frame-interface-digitizer-interleaved-lines
    frame_interface_digitizer__default__get_nbytes,
    frame_interface_digitizer__default__get_nbytes,
    frame_interface_digitizer_interleaved_lines__copy_channel,
    frame_interface_digitizer__default__get_dimensions,
    frame_interface_digitizer__default__get_dimensions
  },
  { frame_interface_resonant__default__get_nchannels,    // 2 - frame-interface-resonant-interleaved-lines
    frame_interface_resonant__default__get_src_nbytes,
    frame_interface_resonant__default__get_dst_nbytes,
    frame_interface_resonant_interleaved_lines__copy_channel,
    frame_interface_resonant__default__get_src_dimensions,
    frame_interface_resonant__default__get_dst_dimensions
  },  
};


//----------------------------------------------------------------------------
// Frame_Descriptor_Get_Interface
//----------------------------------------------------------------------------
extern inline Frame_Interface*
Frame_Descriptor_Get_Interface( Frame_Descriptor *self )
{ int n = sizeof(g_interfaces)/sizeof(Frame_Interface);
#ifdef DEBUG_FRAME_DESCRIPTOR_GET_INTERFACE
  Guarded_Assert( self->interface_id < n );
#endif  
  
  return g_interfaces + self->interface_id;
}

//----------------------------------------------------------------------------
// Frame_Descriptor_Initialize
//----------------------------------------------------------------------------
void
Frame_Descriptor_Change( Frame_Descriptor *self, u8 interface_id, void *metadata, size_t nbytes )
{ self->change_token += 1;
  self->interface_id = interface_id;
  
  Guarded_Assert( nbytes < FRAME_DESCRIPTOR_MAX_METADATA_BYTES );
  memcpy( self->metadata, metadata, nbytes );
  self->metadata_nbytes = nbytes;
}

//----------------------------------------------------------------------------
// Frame_Descriptor_To_File
//----------------------------------------------------------------------------
void
_frame_descriptor_write( FILE *fp, Frame_Descriptor *self, u64 count )
{ Guarded_Assert(1== fwrite( (void*) &self->interface_id,    sizeof( self->interface_id ), 1, fp ) );
  Guarded_Assert(1== fwrite( (void*) &count,                 sizeof( u64 ),                1, fp ) );
  Guarded_Assert(1== fwrite( (void*) &self->metadata_nbytes, sizeof( u32 ),                1, fp ) );
  Guarded_Assert(1== fwrite( (void*) &self->metadata,        self->metadata_nbytes,        1, fp ) );
}

void
Frame_Descriptor_To_File( FILE *fp, Frame_Descriptor *self )
{ size_t count = 1;
  size_t item_bytes = sizeof( self->interface_id ) 
                    + sizeof( count )
                    + self->metadata_nbytes;
  if( self->change_token )                 // add new record
  { _frame_descriptor_write(fp, self, 1 );
  } else
  { Frame_Descriptor last;              // update last record (read and overwrite)
    Guarded_Assert(0== fseek( fp, -(long)item_bytes, SEEK_CUR ) );    
    Guarded_Assert(    Frame_Descriptor_From_File_Read_Next( fp, &last, &count) );
#ifdef DEBUG_FRAME_DESCRIPTOR_TO_FILE
    Guarded_Assert( last.interface_id == self->interface_id );
#endif
    Guarded_Assert(0== fseek( fp, -(long)item_bytes, SEEK_CUR ) );
    _frame_descriptor_write(fp, &last, count+1 );
  }
}

//----------------------------------------------------------------------------
// Frame_Descriptor_From_File_Read_Next
//----------------------------------------------------------------------------
u8
Frame_Descriptor_From_File_Read_Next( FILE *fp, Frame_Descriptor *dst, size_t *repeat_count )
{ if( 1!= fread( &dst->interface_id,    sizeof( dst->interface_id ), 1, fp ) ) goto err;
  if( 1!= fread( repeat_count,          sizeof( u64 ),               1, fp ) ) goto err;
  if( 1!= fread( &dst->metadata_nbytes, sizeof( u32 ),               1, fp ) ) goto err;
  if( 1!= fread( &dst->metadata,        dst->metadata_nbytes,        1, fp ) ) goto err;
  return 1; // success
err:
  Guarded_Assert( feof(fp) );  
  return 0; // failed to read (eof)   
}

//----------------------------------------------------------------------------
// Frame_Get_Size_Bytes
//----------------------------------------------------------------------------
size_t
Frame_Get_Size_Bytes ( Frame_Descriptor *desc )
{ Frame_Interface *face = Frame_Descriptor_Get_Interface(desc);
  size_t nbytes = face->get_source_nbytes(desc);
  return nbytes + sizeof(Frame_Descriptor);
}

//----------------------------------------------------------------------------
// Frame_Cast
//----------------------------------------------------------------------------
void
Frame_Cast( void *bytes, void **data, Frame_Descriptor **desc )
{ *desc = (Frame_Descriptor*) bytes;
  *data = (void*) ((u8*)bytes + sizeof( Frame_Descriptor ));      // MSVS makes you do all this casting....
}

//----------------------------------------------------------------------------
// Frame_Alloc
//----------------------------------------------------------------------------
Frame*
Frame_Alloc( Frame_Descriptor *desc )
{ size_t nbytes = Frame_Get_Size_Bytes( desc );
  Frame_Descriptor *target;
  void *data, 
       *frame = Guarded_Malloc( nbytes, "Frame_Alloc" );  
  Frame_Cast( frame, &data, &target);
  memcpy(target, desc, sizeof(Frame_Descriptor) );
  return frame;
}

//----------------------------------------------------------------------------
// Frame_Free
//----------------------------------------------------------------------------
void
Frame_Free( Frame* frame )
{ if( frame ) free(frame);
}


