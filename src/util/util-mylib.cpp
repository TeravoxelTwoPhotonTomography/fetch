#include "stdafx.h"
#include <string.h>
#include "util-mylib.h"
#include "types.h"

struct CS
{
  CRITICAL_SECTION _cs;
  CS()  {InitializeCriticalSectionAndSpinCount( &_cs, 0x80000400 );}
  ~CS() {DeleteCriticalSection(&_cs);}

  inline void lock()    {EnterCriticalSection(&_cs);}
  inline void unlock()  {LeaveCriticalSection(&_cs);}
};

namespace mylib 
{
  CS _g_cs;
  void lock()   {_g_cs.lock();}
  void unlock() {_g_cs.unlock();}

  static Array_Type frameTypeToArrayType[] = {
    UINT8,//id_u8 = 0,
    UINT16,//id_u16,
    UINT32,//id_u32,
    UINT64,//id_u64,
    INT8,//id_i8,
    INT16,//id_i16,
    INT32,//id_i32,
    INT64,//id_i64,
    FLOAT32,//id_f32,
    FLOAT64//id_f64,
  };

  Array_Type fetchTypeToArrayType(Basic_Type_ID id)
  {
    assert(id != id_unspecified);
    assert(id<MAX_TYPE_ID);
    return frameTypeToArrayType[id];
  }

  static size_t frameTypeToArrayScale[] = {
    8,//id_u8 = 0,
    16,//id_u16,
    32,//id_u32,
    64,//id_u64,
    8,//id_i8,
    16,//id_i16,
    32,//id_i32,
    64,//id_i64,
    32,//id_f32,
    64//id_f64,
  };

  size_t fetchTypeToArrayScale(Basic_Type_ID id)
  {
    assert(id!=id_unspecified);
    assert(id<MAX_TYPE_ID);
    return frameTypeToArrayScale[id];
  }

} //end namespace mylib

namespace mytiff {
  CS _g_cs;
  void lock()   {_g_cs.lock();}
  void unlock() {_g_cs.unlock();}

  Basic_Type_ID pixel_type( Tiff_Image *tim )
  {
    Tiff_Channel *ch0 = tim->channels[0];
    Basic_Type_Attribute q;
    q.bytes = ch0->bytes_per_pixel;
    q.bits  = ch0->scale;
    q.is_signed = ch0->type==CHAN_SIGNED;
    q.is_floating = ch0->type==CHAN_FLOAT;
    long i;    
    for(i=0;i<MAX_TYPE_ID;++i)
    {
      const Basic_Type_Attribute *attr = g_type_attributes+i;
      if(attr->bytes==q.bytes && attr->is_signed==q.is_signed && attr->is_floating==q.is_floating)
        break;
    }
    if(i==MAX_TYPE_ID)
    { warning("Couldn't find a pixel type to match Tiff_Image 0x%p.",tim);
      return id_unspecified;
    }
    return (Basic_Type_ID) i;
  }

  int islsm( const char* fname )
  {
    return strcmp(fname+strlen(fname)-4,".lsm")==0;
  }

} //end namespace mytiff