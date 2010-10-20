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