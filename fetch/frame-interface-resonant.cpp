#pragma once

#include "stdafx.h"
#include "frame.h"
#include "frame-interface-resonant.h"

#if 0
#define DEBUG_FRAME_INTERFACE_RESONANT_INTERLEAVED_LINES
#endif

#ifdef DEBUG_FRAME_INTERFACE_RESONANT_INTERLEAVED_LINES
#define DEBUG_FRAME_INTERFACE_RESONANT_INTERLEAVED_LINES__CHECK_DESCRIPTOR\
  { Guarded_Assert( fd->interface_id    == FRAME_INTERFACE_RESONANT_INTERLEAVED_LINES__INTERFACE_ID );\
    Guarded_Assert( fd->metadata_nbytes == FRAME_INTERFACE_RESONANT_INTERLEAVED_LINES__METADATA_BYTES );\
  }
#else
  #define DEBUG_FRAME_INTERFACE_RESONANT_INTERLEAVED_LINES__CHECK_DESCRIPTOR
#endif

//
// Critical section for this API
// - require for the copy function
//

LPCRITICAL_SECTION gp_critical_section = NULL;

static LPCRITICAL_SECTION _get_critical_section(void)
{ static CRITICAL_SECTION gcs;
  if(!gp_critical_section)
  { InitializeCriticalSectionAndSpinCount( &gcs, 0x80000400 ); // don't assert - Release builds will optimize this out.
    gp_critical_section = &gcs;
  }
  
  return gp_critical_section;
}

//
// Utilities
//

inline u8
_is_format_lut_changed( Resonant_Frame_Metadata *a, Resonant_Frame_Metadata *b)
{ return (a->in_width  != b->in_width ) ||
         (a->in_height != b->in_height) ||
         (a->Bpp       != b->Bpp      ) ||
         (a->rtti      != b->rtti     ) ||
         (a->aspect    != b->aspect   );        
}

inline size_t
compute_out_width( Resonant_Frame_Metadata *format )
{ return (size_t) (2 * format->in_height * format->aspect);
}

inline size_t
compute_out_height( Resonant_Frame_Metadata *format )
{ return (size_t) (2 * format->in_height);
}

//
// Implimentation
//

Basic_Type_ID
frame_interface_resonant__default__get_type (Frame_Descriptor* fd)
{ DEBUG_FRAME_INTERFACE_RESONANT_INTERLEAVED_LINES__CHECK_DESCRIPTOR 
  Resonant_Frame_Metadata *fmt = (Resonant_Frame_Metadata*) fd->metadata;
  return fmt->rtti;
}

void
frame_interface_digitizer__default__set_type( Frame_Descriptor* fd, Basic_Type_ID type)
{ Resonant_Frame_Metadata *meta = (Resonant_Frame_Metadata*) fd->metadata;
  meta->rtti = type;
  meta->Bpp = g_type_attributes[ type ].bytes;
}

size_t
frame_interface_resonant__default__get_nchannels (Frame_Descriptor* fd)
{ DEBUG_FRAME_INTERFACE_RESONANT_INTERLEAVED_LINES__CHECK_DESCRIPTOR 
  Resonant_Frame_Metadata *fmt = (Resonant_Frame_Metadata*) fd->metadata;
  return fmt->nchan;
}

size_t
frame_interface_resonant__default__get_src_nbytes( Frame_Descriptor* fd)
// Returns the number of bytes needed for an output buffer with all channels
{ DEBUG_FRAME_INTERFACE_RESONANT_INTERLEAVED_LINES__CHECK_DESCRIPTOR 
  Resonant_Frame_Metadata *fmt = (Resonant_Frame_Metadata*) fd->metadata;
  return fmt->in_width * fmt->in_height * fmt->nchan * fmt->Bpp;                    // all channels
}

void
frame_interface_resonant__default__get_src_dimensions( Frame_Descriptor* fd, vector_size_t *vdim )
{ DEBUG_FRAME_INTERFACE_RESONANT_INTERLEAVED_LINES__CHECK_DESCRIPTOR
  Resonant_Frame_Metadata *fmt = (Resonant_Frame_Metadata*) fd->metadata;
  vector_size_t_request( vdim, 2 );
  vdim->count       = 2;
  vdim->contents[0] = fmt->in_width;
  vdim->contents[1] = fmt->in_height;
}

size_t
frame_interface_resonant__default__get_dst_nbytes( Frame_Descriptor* fd)
// Returns the number of bytes needed for an output buffer with all channels
{ DEBUG_FRAME_INTERFACE_RESONANT_INTERLEAVED_LINES__CHECK_DESCRIPTOR 
  Resonant_Frame_Metadata *fmt = (Resonant_Frame_Metadata*) fd->metadata;
  return compute_out_height(fmt) * compute_out_width(fmt) * fmt->Bpp;               // single channel
}

void
frame_interface_resonant__default__get_dst_dimensions( Frame_Descriptor* fd, vector_size_t *vdim )
{ DEBUG_FRAME_INTERFACE_RESONANT_INTERLEAVED_LINES__CHECK_DESCRIPTOR
  Resonant_Frame_Metadata *fmt = (Resonant_Frame_Metadata*) fd->metadata;
  vector_size_t_request( vdim, 2 );
  vdim->count       = 2;
  vdim->contents[0] = compute_out_width(fmt);
  vdim->contents[1] = compute_out_height(fmt);
}

void
frame_interface_resonant_interleaved_lines__copy_channel( Frame_Descriptor* fd, void *dst, size_t dst_stride, void *src, size_t ichan )
{ static vector_u16                   *vlut = NULL,
                                      *vacc = NULL;
  static Resonant_Frame_Metadata      *last = NULL;
  static int                     turnaround = 0;
#ifdef DEBUG_FRAME_INTERFACE_RESONANT_INTERLEAVED_LINES
  static int                     iter_count = 0;
#endif
  size_t out_width, out_height;
  Resonant_Frame_Metadata *fmt = (Resonant_Frame_Metadata*) fd->metadata;   
  DEBUG_FRAME_INTERFACE_RESONANT_INTERLEAVED_LINES__CHECK_DESCRIPTOR
  
  out_width  = compute_out_width(  fmt );
  out_height = compute_out_height( fmt );
  
  //
  //  Compute Look up table
  //
  
  EnterCriticalSection( _get_critical_section() );
  { if( !vlut )
    { vlut = vector_u16_alloc( 0 );
      vacc = vector_u16_alloc( 0 );      
    }
    if( !last || _is_format_lut_changed( last, fmt ) )
    { // Something changed, Recompute lut and acc
      f64 k = 2.0*M_PI* (fmt->duty/(double) fmt->in_width ),
          A = out_width/2.0;
      vector_u16_request( vlut, fmt->in_width );
      vector_u16_request( vacc, 2*out_width   );
      { int i = fmt->in_width;
        u16 *lut = vlut->contents,
            *acc = vacc->contents;
        int Bpp  = fmt->Bpp;
        memset(lut, 0, vlut->nelem * vlut->bytes_per_elem);
        memset(acc, 0, vacc->nelem * vacc->bytes_per_elem);
        while(i--)
        { double x = k*i;
          int j = (int) ( A * (1.0 - cos( x )) );                    
          if( x >= M_PI )     // encode switch to next line as an offset by out_width
            j += out_width;
          lut[i] = j*Bpp;    // when lut[i] >= Bpp*out_width, the scan is on the next line
          acc[j]++;
        }
        // Find turnaround index
        { int i = fmt->in_width;
          while(i--)                     // going backwards to start on the next line
            if( lut[i] < Bpp*out_width ) // ...we're done when hit next line          
              break;
          turnaround = i + 1;
          assert( turnaround != fmt->in_width ); // TEST
          assert( turnaround != 0 );             // TEST
        }
      } // end lut access scope
#ifdef DEBUG_FRAME_INTERFACE_RESONANT_INTERLEAVED_LINES
      { FILE *fp = fopen("lut.raw","wb");
        fwrite( vlut->contents, vlut->bytes_per_elem, vlut->nelem, fp );
        fclose(fp);
      }
      { FILE *fp = fopen("acc.raw","wb");
        fwrite( vacc->contents, vacc->bytes_per_elem, vacc->nelem, fp );
        fclose(fp);
      }
#endif
    } // end "if changed" response    

  
    //
    //  Perform copy
    //  Apply Look up table
    //

    { size_t src_chan_stride = fmt->in_width * fmt->Bpp,
             src_scan_stride = fmt->nchan * src_chan_stride,
             dst_scan_stride = 2 * dst_stride,
             nbytes          = MIN( dst_stride, out_width*fmt->Bpp );         // bytes to copy for a line
      u8    *src_beg         = (u8*)src   + ichan * src_chan_stride,          // selects the source channel
            *src_cur         = src_beg    + src_scan_stride * fmt->in_height, // set source cursor at (one past) end
            *dst_cur         = (u8*)dst   + dst_scan_stride * fmt->in_height;
      int    delta           = dst_stride - out_width*fmt->Bpp;               // shift to next line corrected for lut encoding of next line
      u16   *lut = vlut->contents,
            *acc = vacc->contents;
      size_t Bpp = fmt->Bpp;
  #ifdef DEBUG_FRAME_INTERFACE_RESONANT_INTERLEAVED_LINES
      { char filename[128] = "source.raw";
        FILE *fp;
        //sprintf(filename, "source%03d.raw", iter_count++);
        fp = fopen(filename,"wb");    
        fwrite( src, 1, fmt->in_width * fmt->in_height * fmt->nchan * fmt->Bpp, fp );
        fclose(fp); 
      }
  #endif          
      while( src_cur > src_beg )
      { u16 i;      
        src_cur -= src_scan_stride;
        dst_cur -= dst_scan_stride;
        
        memset(dst_cur,             0,nbytes);
        memset(dst_cur + dst_stride,0,nbytes);
        
        // COPY
        for( i=0; i<turnaround; i++ )             // First line
          memcpy( dst_cur + lut[i], src_cur + Bpp*i, Bpp );
                 
        for( ; i<fmt->in_width; i++ )             // Second line
          memcpy( dst_cur + lut[i] + delta, src_cur + Bpp*i, Bpp );
        
        //// Accumulate                                                       // FIXME: Risk Overflow
        //for( i=0; i<turnaround; i++ )             // First line
        //  dst_cur[ lut[i] ] += src_cur[i];
        //         
        //for( ; i<fmt->in_width; i++ )             // Second line
        //  dst_cur[ lut[i] + delta ] += src_cur[i];
          
        //// Average
        //for( i=0; i<out_width; i++ )             // First line
        //  if( acc[i] )
        //    dst_cur[ i ] /= acc[i];
        //         
        //for( ; i<(2*out_width); i++ )              // Second line
        //  if( acc[i] )
        //    dst_cur[ i + dst_stride ] /= acc[i];
        
      }
  #ifdef DEBUG_FRAME_INTERFACE_RESONANT_INTERLEAVED_LINES
      { FILE *fp = fopen("dest.raw","wb");    
        fwrite( dst, 1, dst_stride * out_height, fp );
        fclose(fp); 
      }
  #endif    
    } // end copy scope
    
  } // end CriticalSection scope
  LeaveCriticalSection( _get_critical_section() );
}
