#include "StdAfx.h"
#include "FrameInvert.h"

namespace fetch {
namespace task {

  template<class T> void invert(T* data, size_t nbytes)
  { 
    size_t n = nbytes/sizeof(T);
    for(size_t i=0; i<n; ++i, ++data)
      (*data) = -(*data);      
  }
  
  template<class T> void uinvert(T* data, size_t nbytes)
  { T max = (T) -1; // T is unsigned
    size_t n = nbytes/sizeof(T);
    
    for(size_t i=0; i<n; ++i, ++data)
      (*data) = max-(*data);      
  }
  
  unsigned int
  FrameInvert::
  work(IDevice *idc, Frame *fsrc)
  { worker::FrameInvertAgent *dc = dynamic_cast<worker::FrameInvertAgent*>(idc);
    void  *s;
    size_t src_pitch[4], n[3];
    fsrc->compute_pitches(src_pitch);
    fsrc->get_shape(n);
    
    //debug("In FrameInvert::work.\r\n");      
    //fsrc->dump("FrameInvert-src.%s",TypeStrFromID(fsrc->rtti));
    
    s = fsrc->data;   
    switch (fsrc->rtti) {
      case id_u8 : uinvert<u8 >((u8* )s,src_pitch[0]); break;
      case id_u16: uinvert<u16>((u16*)s,src_pitch[0]); break;
      case id_u32: uinvert<u32>((u32*)s,src_pitch[0]); break;
      case id_u64: uinvert<u64>((u64*)s,src_pitch[0]); break;
      case id_i8 : invert<i8 >((i8* )s,src_pitch[0]); break;
      case id_i16: invert<i16>((i16*)s,src_pitch[0]); break;
      case id_i32: invert<i32>((i32*)s,src_pitch[0]); break;
      case id_i64: invert<i64>((i64*)s,src_pitch[0]); break;
      case id_f32: invert<f32>((f32*)s,src_pitch[0]); break;
      case id_f64: invert<f64>((f64*)s,src_pitch[0]); break;
      default:
        warning("Could not interpret source type.\r\n");
        return 0; // failure
        break;
    }
    
    //fsrc->dump("FrameInvert-src.%s",TypeStrFromID(fsrc->rtti));
    
    return 1; // success
  }
    
  void
  FrameInvert::
  alloc_output_queues(IDevice *dc)
  { // Allocates an output queue on out[0] that has matching storage to in[0].      
    dc->_alloc_qs_easy(&dc->_out,
                          1,                                               // number of output channels to allocate
                          dc->_in->contents[0]->q->ring->nelem,            // copy number of output buffers from input queue
                          dc->_in->contents[0]->q->buffer_size_bytes);
  }
  
}
}