#include "../fetch/util-image.h"

/*
 * Exposes the following functions:
 *
      void imCastCopy_??_??     ( Tdst *dst, size_t dst_pitch[4], Tsrc *src, size_t src_pitch[4], size_t shape[3] )
      void imCopy_??_??         ( Tdst *dst, size_t dst_pitch[4], Tsrc *src, size_t src_pitch[4], size_t shape[3] )
      void imCopyTranspose_??_??( Tdst *dst, size_t dst_pitch[4], Tsrc *src, size_t src_pitch[4], size_t shape[3], int i, int j )
 *
 * where the ?? are any of the supported pixel types: u8, u16, u32, u64,
 *                                                    i8, i16, i32, i64,
 *                                                             f32, f64
 *
 */

// -----------------
// fixed width types
// -----------------
typedef unsigned char       u8;
typedef unsigned short     u16;
typedef unsigned int       u32;
typedef unsigned long long u64;
typedef char               i8;
typedef short              i16;
typedef int                i32;
typedef long long          i64;
typedef float              f32;
typedef double             f64;

#define STUB(f,t0,t1) f##_##t0##_##t1

#define STUB_IM_CAST_COPY(Tdst,Tsrc)\
  extern "C" void STUB(imCastCopy,Tdst,Tsrc)( Tdst *dst, size_t dst_pitch[4], Tsrc *src, size_t src_pitch[4], size_t n[3] )\
  { imCastCopy<Tdst,Tsrc>(dst,dst_pitch,src,src_pitch,n); }

STUB_IM_CAST_COPY(u8 ,u8 )
STUB_IM_CAST_COPY(u8 ,u16)
STUB_IM_CAST_COPY(u8 ,u32)
STUB_IM_CAST_COPY(u8 ,u64)
STUB_IM_CAST_COPY(u8 ,i8 )
STUB_IM_CAST_COPY(u8 ,i16)
STUB_IM_CAST_COPY(u8 ,i32)
STUB_IM_CAST_COPY(u8 ,i64)
STUB_IM_CAST_COPY(u8 ,f32)
STUB_IM_CAST_COPY(u8 ,f64)

STUB_IM_CAST_COPY(u16,u8 )
STUB_IM_CAST_COPY(u16,u16)
STUB_IM_CAST_COPY(u16,u32)
STUB_IM_CAST_COPY(u16,u64)
STUB_IM_CAST_COPY(u16,i8 )
STUB_IM_CAST_COPY(u16,i16)
STUB_IM_CAST_COPY(u16,i32)
STUB_IM_CAST_COPY(u16,i64)
STUB_IM_CAST_COPY(u16,f32)
STUB_IM_CAST_COPY(u16,f64)

STUB_IM_CAST_COPY(u32,u8 )
STUB_IM_CAST_COPY(u32,u16)
STUB_IM_CAST_COPY(u32,u32)
STUB_IM_CAST_COPY(u32,u64)
STUB_IM_CAST_COPY(u32,i8 )
STUB_IM_CAST_COPY(u32,i16)
STUB_IM_CAST_COPY(u32,i32)
STUB_IM_CAST_COPY(u32,i64)
STUB_IM_CAST_COPY(u32,f32)
STUB_IM_CAST_COPY(u32,f64)

STUB_IM_CAST_COPY(u64,u8 )
STUB_IM_CAST_COPY(u64,u16)
STUB_IM_CAST_COPY(u64,u32)
STUB_IM_CAST_COPY(u64,u64)
STUB_IM_CAST_COPY(u64,i8 )
STUB_IM_CAST_COPY(u64,i16)
STUB_IM_CAST_COPY(u64,i32)
STUB_IM_CAST_COPY(u64,i64)
STUB_IM_CAST_COPY(u64,f32)
STUB_IM_CAST_COPY(u64,f64)

STUB_IM_CAST_COPY(i8 ,u8 )
STUB_IM_CAST_COPY(i8 ,u16)
STUB_IM_CAST_COPY(i8 ,u32)
STUB_IM_CAST_COPY(i8 ,u64)
STUB_IM_CAST_COPY(i8 ,i8 )
STUB_IM_CAST_COPY(i8 ,i16)
STUB_IM_CAST_COPY(i8 ,i32)
STUB_IM_CAST_COPY(i8 ,i64)
STUB_IM_CAST_COPY(i8 ,f32)
STUB_IM_CAST_COPY(i8 ,f64)

STUB_IM_CAST_COPY(i16,u8 )
STUB_IM_CAST_COPY(i16,u16)
STUB_IM_CAST_COPY(i16,u32)
STUB_IM_CAST_COPY(i16,u64)
STUB_IM_CAST_COPY(i16,i8 )
STUB_IM_CAST_COPY(i16,i16)
STUB_IM_CAST_COPY(i16,i32)
STUB_IM_CAST_COPY(i16,i64)
STUB_IM_CAST_COPY(i16,f32)
STUB_IM_CAST_COPY(i16,f64)

STUB_IM_CAST_COPY(i32,u8 )
STUB_IM_CAST_COPY(i32,u16)
STUB_IM_CAST_COPY(i32,u32)
STUB_IM_CAST_COPY(i32,u64)
STUB_IM_CAST_COPY(i32,i8 )
STUB_IM_CAST_COPY(i32,i16)
STUB_IM_CAST_COPY(i32,i32)
STUB_IM_CAST_COPY(i32,i64)
STUB_IM_CAST_COPY(i32,f32)
STUB_IM_CAST_COPY(i32,f64)

STUB_IM_CAST_COPY(i64,u8 )
STUB_IM_CAST_COPY(i64,u16)
STUB_IM_CAST_COPY(i64,u32)
STUB_IM_CAST_COPY(i64,u64)
STUB_IM_CAST_COPY(i64,i8 )
STUB_IM_CAST_COPY(i64,i16)
STUB_IM_CAST_COPY(i64,i32)
STUB_IM_CAST_COPY(i64,i64)
STUB_IM_CAST_COPY(i64,f32)
STUB_IM_CAST_COPY(i64,f64)

STUB_IM_CAST_COPY(f32,u8 )
STUB_IM_CAST_COPY(f32,u16)
STUB_IM_CAST_COPY(f32,u32)
STUB_IM_CAST_COPY(f32,u64)
STUB_IM_CAST_COPY(f32,i8 )
STUB_IM_CAST_COPY(f32,i16)
STUB_IM_CAST_COPY(f32,i32)
STUB_IM_CAST_COPY(f32,i64)
STUB_IM_CAST_COPY(f32,f32)
STUB_IM_CAST_COPY(f32,f64)

STUB_IM_CAST_COPY(f64,u8 )
STUB_IM_CAST_COPY(f64,u16)
STUB_IM_CAST_COPY(f64,u32)
STUB_IM_CAST_COPY(f64,u64)
STUB_IM_CAST_COPY(f64,i8 )
STUB_IM_CAST_COPY(f64,i16)
STUB_IM_CAST_COPY(f64,i32)
STUB_IM_CAST_COPY(f64,i64)
STUB_IM_CAST_COPY(f64,f32)
STUB_IM_CAST_COPY(f64,f64)

#define STUB_IM_COPY(Tdst,Tsrc)\
  extern "C" void STUB(imCopy,Tdst,Tsrc)( Tdst *dst, size_t dst_pitch[4], Tsrc *src, size_t src_pitch[4], size_t n[3] )\
  { imCopy<Tdst,Tsrc>(dst,dst_pitch,src,src_pitch,n); }

STUB_IM_COPY(u8 ,u8 )
STUB_IM_COPY(u8 ,u16)
STUB_IM_COPY(u8 ,u32)
STUB_IM_COPY(u8 ,u64)
STUB_IM_COPY(u8 ,i8 )
STUB_IM_COPY(u8 ,i16)
STUB_IM_COPY(u8 ,i32)
STUB_IM_COPY(u8 ,i64)
STUB_IM_COPY(u8 ,f32)
STUB_IM_COPY(u8 ,f64)

STUB_IM_COPY(u16,u8 )
STUB_IM_COPY(u16,u16)
STUB_IM_COPY(u16,u32)
STUB_IM_COPY(u16,u64)
STUB_IM_COPY(u16,i8 )
STUB_IM_COPY(u16,i16)
STUB_IM_COPY(u16,i32)
STUB_IM_COPY(u16,i64)
STUB_IM_COPY(u16,f32)
STUB_IM_COPY(u16,f64)

STUB_IM_COPY(u32,u8 )
STUB_IM_COPY(u32,u16)
STUB_IM_COPY(u32,u32)
STUB_IM_COPY(u32,u64)
STUB_IM_COPY(u32,i8 )
STUB_IM_COPY(u32,i16)
STUB_IM_COPY(u32,i32)
STUB_IM_COPY(u32,i64)
STUB_IM_COPY(u32,f32)
STUB_IM_COPY(u32,f64)

STUB_IM_COPY(u64,u8 )
STUB_IM_COPY(u64,u16)
STUB_IM_COPY(u64,u32)
STUB_IM_COPY(u64,u64)
STUB_IM_COPY(u64,i8 )
STUB_IM_COPY(u64,i16)
STUB_IM_COPY(u64,i32)
STUB_IM_COPY(u64,i64)
STUB_IM_COPY(u64,f32)
STUB_IM_COPY(u64,f64)

STUB_IM_COPY(i8 ,u8 )
STUB_IM_COPY(i8 ,u16)
STUB_IM_COPY(i8 ,u32)
STUB_IM_COPY(i8 ,u64)
STUB_IM_COPY(i8 ,i8 )
STUB_IM_COPY(i8 ,i16)
STUB_IM_COPY(i8 ,i32)
STUB_IM_COPY(i8 ,i64)
STUB_IM_COPY(i8 ,f32)
STUB_IM_COPY(i8 ,f64)

STUB_IM_COPY(i16,u8 )
STUB_IM_COPY(i16,u16)
STUB_IM_COPY(i16,u32)
STUB_IM_COPY(i16,u64)
STUB_IM_COPY(i16,i8 )
STUB_IM_COPY(i16,i16)
STUB_IM_COPY(i16,i32)
STUB_IM_COPY(i16,i64)
STUB_IM_COPY(i16,f32)
STUB_IM_COPY(i16,f64)

STUB_IM_COPY(i32,u8 )
STUB_IM_COPY(i32,u16)
STUB_IM_COPY(i32,u32)
STUB_IM_COPY(i32,u64)
STUB_IM_COPY(i32,i8 )
STUB_IM_COPY(i32,i16)
STUB_IM_COPY(i32,i32)
STUB_IM_COPY(i32,i64)
STUB_IM_COPY(i32,f32)
STUB_IM_COPY(i32,f64)

STUB_IM_COPY(i64,u8 )
STUB_IM_COPY(i64,u16)
STUB_IM_COPY(i64,u32)
STUB_IM_COPY(i64,u64)
STUB_IM_COPY(i64,i8 )
STUB_IM_COPY(i64,i16)
STUB_IM_COPY(i64,i32)
STUB_IM_COPY(i64,i64)
STUB_IM_COPY(i64,f32)
STUB_IM_COPY(i64,f64)

STUB_IM_COPY(f32,u8 )
STUB_IM_COPY(f32,u16)
STUB_IM_COPY(f32,u32)
STUB_IM_COPY(f32,u64)
STUB_IM_COPY(f32,i8 )
STUB_IM_COPY(f32,i16)
STUB_IM_COPY(f32,i32)
STUB_IM_COPY(f32,i64)
STUB_IM_COPY(f32,f32)
STUB_IM_COPY(f32,f64)

STUB_IM_COPY(f64,u8 )
STUB_IM_COPY(f64,u16)
STUB_IM_COPY(f64,u32)
STUB_IM_COPY(f64,u64)
STUB_IM_COPY(f64,i8 )
STUB_IM_COPY(f64,i16)
STUB_IM_COPY(f64,i32)
STUB_IM_COPY(f64,i64)
STUB_IM_COPY(f64,f32)
STUB_IM_COPY(f64,f64)

#define STUB_IM_COPY_TRANSPOSE(Tdst,Tsrc)\
  extern "C" void STUB(imCopyTranspose,Tdst,Tsrc)( Tdst *dst, size_t dst_pitch[4], Tsrc *src, size_t src_pitch[4], size_t shape[3], int i, int j )\
  { imCopyTranspose<Tdst,Tsrc>(dst,dst_pitch,src,src_pitch,shape,i,j); }

STUB_IM_COPY_TRANSPOSE(u8 ,u8 )
STUB_IM_COPY_TRANSPOSE(u8 ,u16)
STUB_IM_COPY_TRANSPOSE(u8 ,u32)
STUB_IM_COPY_TRANSPOSE(u8 ,u64)
STUB_IM_COPY_TRANSPOSE(u8 ,i8 )
STUB_IM_COPY_TRANSPOSE(u8 ,i16)
STUB_IM_COPY_TRANSPOSE(u8 ,i32)
STUB_IM_COPY_TRANSPOSE(u8 ,i64)
STUB_IM_COPY_TRANSPOSE(u8 ,f32)
STUB_IM_COPY_TRANSPOSE(u8 ,f64)

STUB_IM_COPY_TRANSPOSE(u16,u8 )
STUB_IM_COPY_TRANSPOSE(u16,u16)
STUB_IM_COPY_TRANSPOSE(u16,u32)
STUB_IM_COPY_TRANSPOSE(u16,u64)
STUB_IM_COPY_TRANSPOSE(u16,i8 )
STUB_IM_COPY_TRANSPOSE(u16,i16)
STUB_IM_COPY_TRANSPOSE(u16,i32)
STUB_IM_COPY_TRANSPOSE(u16,i64)
STUB_IM_COPY_TRANSPOSE(u16,f32)
STUB_IM_COPY_TRANSPOSE(u16,f64)

STUB_IM_COPY_TRANSPOSE(u32,u8 )
STUB_IM_COPY_TRANSPOSE(u32,u16)
STUB_IM_COPY_TRANSPOSE(u32,u32)
STUB_IM_COPY_TRANSPOSE(u32,u64)
STUB_IM_COPY_TRANSPOSE(u32,i8 )
STUB_IM_COPY_TRANSPOSE(u32,i16)
STUB_IM_COPY_TRANSPOSE(u32,i32)
STUB_IM_COPY_TRANSPOSE(u32,i64)
STUB_IM_COPY_TRANSPOSE(u32,f32)
STUB_IM_COPY_TRANSPOSE(u32,f64)

STUB_IM_COPY_TRANSPOSE(u64,u8 )
STUB_IM_COPY_TRANSPOSE(u64,u16)
STUB_IM_COPY_TRANSPOSE(u64,u32)
STUB_IM_COPY_TRANSPOSE(u64,u64)
STUB_IM_COPY_TRANSPOSE(u64,i8 )
STUB_IM_COPY_TRANSPOSE(u64,i16)
STUB_IM_COPY_TRANSPOSE(u64,i32)
STUB_IM_COPY_TRANSPOSE(u64,i64)
STUB_IM_COPY_TRANSPOSE(u64,f32)
STUB_IM_COPY_TRANSPOSE(u64,f64)

STUB_IM_COPY_TRANSPOSE(i8 ,u8 )
STUB_IM_COPY_TRANSPOSE(i8 ,u16)
STUB_IM_COPY_TRANSPOSE(i8 ,u32)
STUB_IM_COPY_TRANSPOSE(i8 ,u64)
STUB_IM_COPY_TRANSPOSE(i8 ,i8 )
STUB_IM_COPY_TRANSPOSE(i8 ,i16)
STUB_IM_COPY_TRANSPOSE(i8 ,i32)
STUB_IM_COPY_TRANSPOSE(i8 ,i64)
STUB_IM_COPY_TRANSPOSE(i8 ,f32)
STUB_IM_COPY_TRANSPOSE(i8 ,f64)

STUB_IM_COPY_TRANSPOSE(i16,u8 )
STUB_IM_COPY_TRANSPOSE(i16,u16)
STUB_IM_COPY_TRANSPOSE(i16,u32)
STUB_IM_COPY_TRANSPOSE(i16,u64)
STUB_IM_COPY_TRANSPOSE(i16,i8 )
STUB_IM_COPY_TRANSPOSE(i16,i16)
STUB_IM_COPY_TRANSPOSE(i16,i32)
STUB_IM_COPY_TRANSPOSE(i16,i64)
STUB_IM_COPY_TRANSPOSE(i16,f32)
STUB_IM_COPY_TRANSPOSE(i16,f64)

STUB_IM_COPY_TRANSPOSE(i32,u8 )
STUB_IM_COPY_TRANSPOSE(i32,u16)
STUB_IM_COPY_TRANSPOSE(i32,u32)
STUB_IM_COPY_TRANSPOSE(i32,u64)
STUB_IM_COPY_TRANSPOSE(i32,i8 )
STUB_IM_COPY_TRANSPOSE(i32,i16)
STUB_IM_COPY_TRANSPOSE(i32,i32)
STUB_IM_COPY_TRANSPOSE(i32,i64)
STUB_IM_COPY_TRANSPOSE(i32,f32)
STUB_IM_COPY_TRANSPOSE(i32,f64)

STUB_IM_COPY_TRANSPOSE(i64,u8 )
STUB_IM_COPY_TRANSPOSE(i64,u16)
STUB_IM_COPY_TRANSPOSE(i64,u32)
STUB_IM_COPY_TRANSPOSE(i64,u64)
STUB_IM_COPY_TRANSPOSE(i64,i8 )
STUB_IM_COPY_TRANSPOSE(i64,i16)
STUB_IM_COPY_TRANSPOSE(i64,i32)
STUB_IM_COPY_TRANSPOSE(i64,i64)
STUB_IM_COPY_TRANSPOSE(i64,f32)
STUB_IM_COPY_TRANSPOSE(i64,f64)

STUB_IM_COPY_TRANSPOSE(f32,u8 )
STUB_IM_COPY_TRANSPOSE(f32,u16)
STUB_IM_COPY_TRANSPOSE(f32,u32)
STUB_IM_COPY_TRANSPOSE(f32,u64)
STUB_IM_COPY_TRANSPOSE(f32,i8 )
STUB_IM_COPY_TRANSPOSE(f32,i16)
STUB_IM_COPY_TRANSPOSE(f32,i32)
STUB_IM_COPY_TRANSPOSE(f32,i64)
STUB_IM_COPY_TRANSPOSE(f32,f32)
STUB_IM_COPY_TRANSPOSE(f32,f64)

STUB_IM_COPY_TRANSPOSE(f64,u8 )
STUB_IM_COPY_TRANSPOSE(f64,u16)
STUB_IM_COPY_TRANSPOSE(f64,u32)
STUB_IM_COPY_TRANSPOSE(f64,u64)
STUB_IM_COPY_TRANSPOSE(f64,i8 )
STUB_IM_COPY_TRANSPOSE(f64,i16)
STUB_IM_COPY_TRANSPOSE(f64,i32)
STUB_IM_COPY_TRANSPOSE(f64,i64)
STUB_IM_COPY_TRANSPOSE(f64,f32)
STUB_IM_COPY_TRANSPOSE(f64,f64)