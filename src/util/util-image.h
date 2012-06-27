#pragma once

#include <string.h>
#include <stdio.h>
#include "../types.h"

// N-dimensions
// Optimized copy ops could go N-dimensional using recursion.


// Source and destination must have the same meaning assigned to
// dimensions.

// Pitches are defined from largest to smallest.  For example:
//  pitch = [ width * height * nchannels * pp,  // dimension 3 - full  size
//                    height * nchannels * pp,  // dimension 2 - plane size
//                             nchannels * pp,  // dimension 1 - line  size
//                                         pp]  // dimension 0 - pixel size
//  "pp" stands for pixel pitch - # of elements per pixel
//
// Where possible we'd like to copy contiguous blocks of data.
// This is possible when for blocks less than dimension d if:
//   1. source and destination pixel types are the same
//   2. the dimension zero stride is the size of the pixel types.
//   3. the strides up to dimension d are matched
// 
// Using pitches allows one to flexibly specify a subregion of an
// image.

// n is the number of elements to copy in each dimension.  These dimensions
// are assumed to fit.  To copy a RGB 256x512 image (row x cols), use:
//   
//  n[] = { 256, 512, 3 };

#ifdef __cplusplus
#define mydeclspec extern "C"
#else
#define mydeclspec
#endif

mydeclspec
void Compute_Pitch( size_t pitch[4], size_t d2, size_t d1, size_t d0, size_t pixel_pitch);

int _pitch_first_mismatch( size_t dst_pitch[4], size_t src_pitch[4] );

#ifdef __cplusplus
// cast and copy - always uses a slow copy to gaurantee casting
//
// - divide pitches by sizeof types since the copy/indexing implicitly
//   accounts for this step size.
template<class Tdst,class Tsrc>
void
imCastCopy( Tdst *dst, size_t dst_pitch[4], Tsrc *src, size_t src_pitch[4], size_t n[3] )
{ size_t i,j,k,
          bs = sizeof(Tsrc),
          bd = sizeof(Tdst),
          dp0 = dst_pitch[1]/bd, // the biggest jump
          dp1 = dst_pitch[2]/bd,
          dp2 = dst_pitch[3]/bd, // the smallest jump
          sp0 = src_pitch[1]/bs,
          sp1 = src_pitch[2]/bs,
          sp2 = src_pitch[3]/bs;
  for(i=0;i<n[0];++i)
    for(j=0;j<n[1];++j)
      for(k=0;k<n[2];++k)
        dst[dp0*i + dp1*j + dp2*k] = Saturate<Tdst,Tsrc>(src[sp0*i + sp1*j + sp2*k]);
}
#endif // #ifdef __cplusplus

#define DECL_IMCASTCOPY(Tdstlbl,Tdst,Tsrclbl,Tsrc) \
  mydeclspec void imCastCopy_##Tdstlbl##_##Tsrclbl( Tdst *dst, size_t dst_pitch[4], Tsrc *src, size_t src_pitch[4], size_t n[3] );


#define DECL_IMCASTCOPY_BY_DEST(Tdstlbl,Tdst) \
  DECL_IMCASTCOPY(Tdstlbl,Tdst ,u8 ,u8  );\
  DECL_IMCASTCOPY(Tdstlbl,Tdst ,u16,u16 );\
  DECL_IMCASTCOPY(Tdstlbl,Tdst ,u32,u32 );\
  DECL_IMCASTCOPY(Tdstlbl,Tdst ,u64,u64 );\
  DECL_IMCASTCOPY(Tdstlbl,Tdst ,i8 , i8  );\
  DECL_IMCASTCOPY(Tdstlbl,Tdst ,i16, i16 );\
  DECL_IMCASTCOPY(Tdstlbl,Tdst ,i32, i32 );\
  DECL_IMCASTCOPY(Tdstlbl,Tdst ,i64, i64 );\
  DECL_IMCASTCOPY(Tdstlbl,Tdst ,f32, f32   );\
  DECL_IMCASTCOPY(Tdstlbl,Tdst ,f64, f64  );

DECL_IMCASTCOPY_BY_DEST( u8 ,u8  );
DECL_IMCASTCOPY_BY_DEST( u16,u16 );
DECL_IMCASTCOPY_BY_DEST( u32,u32 );
DECL_IMCASTCOPY_BY_DEST( u64,u64 );
DECL_IMCASTCOPY_BY_DEST( i8 , i8  );
DECL_IMCASTCOPY_BY_DEST( i16, i16 );
DECL_IMCASTCOPY_BY_DEST( i32, i32 );
DECL_IMCASTCOPY_BY_DEST( i64, i64 );
DECL_IMCASTCOPY_BY_DEST( f32, f32   );
DECL_IMCASTCOPY_BY_DEST( f64, f64  );

#ifdef __cplusplus
// copies - casting not gauranteed
template<class Tdst, class Tsrc>
void
imCopy( Tdst *dst, size_t dst_pitch[4], Tsrc *src, size_t src_pitch[4], size_t n[3] )
{ int mismatch = _pitch_first_mismatch(dst_pitch, src_pitch);
  size_t bs = sizeof(Tsrc),
         bd = sizeof(Tdst),         
         vols[3] = { n[0]*n[1]*n[2]*bs,  // in bytes.  used in block copies
                          n[1]*n[2]*bs, 
                               n[2]*bs};
  if( sizeof(Tdst) != sizeof(Tsrc) )
    mismatch = 3; //a mismatch on the pixel level will force the pixel by pixel copy

  switch( mismatch )
  { case -1: // all strides match
    case  0: // all but top level strides match - that is, only the total size differs
      memcpy( dst, src, vols[0] );
      break;
    case  1:                        // "planes" as chunks
      { size_t i,                   // number of planes
               v = vols[1],         // number of elements in a chunk
               dp = dst_pitch[1],   // plane pitch
               sp = src_pitch[1];   //
        for(i=0;i<n[0];++i)
          memcpy(dst+dp*i,src+sp*i,v);
      }
      break;
    case  2:                        // "line" as chunks
      { size_t i,j,                 // number of planes
               v   = vols[2],       // number of elements in a chunk
               dp0 = dst_pitch[1],  // plane pitch
               dp1 = dst_pitch[2],  // line pitch
               sp0 = src_pitch[1],  // plane pitch
               sp1 = src_pitch[2];  // line pitch
        for(i=0;i<n[0];++i)
          for(j=0;j<n[1];++j)
            memcpy(dst+dp0*i+dp1*j,
                   src+sp0*i+sp1*j,
                   v);
      }
      break;
    case  3:                        // pixel copy
    default:
      { size_t k,j,i = n[0],        // number of planes
               dp0 = dst_pitch[1]/bd,  // plane pitch - device these by pixel size since copy op implicitly includes pixel step
               dp1 = dst_pitch[2]/bd,  // line  pitch
               dp2 = dst_pitch[3]/bd,  // pixel pitch
               sp0 = src_pitch[1]/bs,
               sp1 = src_pitch[2]/bs,
               sp2 = src_pitch[3]/bs;
        for(i=0;i<n[0];++i)
          for(j=0;j<n[1];++j)
            for(k=0;k<n[2];++k)
#pragma warning( push )
#pragma warning( disable:4244 )
              dst[dp0*i + dp1*j + dp2*k] = Saturate<Tdst,Tsrc>(src[sp0*i + sp1*j + sp2*k]);
#pragma warning( pop )              
      }
      break;
  }
}
#endif //#ifdef __cplusplus

#define DECL_IMCOPY(Tdstlbl,Tdst,Tsrclbl,Tsrc) \
  mydeclspec void imCopy_##Tdstlbl##_##Tsrclbl( Tdst *dst, size_t dst_pitch[4], Tsrc *src, size_t src_pitch[4], size_t n[3] );

#define DECL_IMCOPY_BY_DEST(Tdstlbl,Tdst) \
  DECL_IMCOPY(Tdstlbl,Tdst ,u8 ,u8  );\
  DECL_IMCOPY(Tdstlbl,Tdst ,u16,u16 );\
  DECL_IMCOPY(Tdstlbl,Tdst ,u32,u32 );\
  DECL_IMCOPY(Tdstlbl,Tdst ,u64,u64 );\
  DECL_IMCOPY(Tdstlbl,Tdst ,i8 , i8  );\
  DECL_IMCOPY(Tdstlbl,Tdst ,i16, i16 );\
  DECL_IMCOPY(Tdstlbl,Tdst ,i32, i32 );\
  DECL_IMCOPY(Tdstlbl,Tdst ,i64, i64 );\
  DECL_IMCOPY(Tdstlbl,Tdst ,f32, f32   );\
  DECL_IMCOPY(Tdstlbl,Tdst ,f64, f64  );

DECL_IMCOPY_BY_DEST( u8 ,u8  );
DECL_IMCOPY_BY_DEST( u16,u16 );
DECL_IMCOPY_BY_DEST( u32,u32 );
DECL_IMCOPY_BY_DEST( u64,u64 );
DECL_IMCOPY_BY_DEST( i8 , i8  );
DECL_IMCOPY_BY_DEST( i16, i16 );
DECL_IMCOPY_BY_DEST( i32, i32 );
DECL_IMCOPY_BY_DEST( i64, i64 );
DECL_IMCOPY_BY_DEST( f32, f32   );
DECL_IMCOPY_BY_DEST( f64, f64  );

// Copy Transpose
//
// Transposes dimensions i and j during a copy.
//
// <i>,<j> should be 0, 1, or 2 (for an interlaced rgba image this would be plane,line,pixel)
//                              ( or for depth, row, column )
// <shape> should be specified as the shape in the source space,
//         that is, before transposition.  It's returned
//         in the destination (transposed) space.
//
//  <src_pitch> should specifies the source dimensions.  See Compute_Pitch.
//  <dst_pitch> should specifies the dest   dimensions.
//

// This works as follows:
//
// A positional vector, r, in {whole numbers}^N gets mapped to an index with a
// linear projection.  For example, in the <src> array the value at r is:
//
//   src[p.r]
//
// where p is the projection for <src>.
//
// We might want to copy values from a <src> array to a <dst> array.
//
//   dst[p'.r'] = src[p.r]
//
// where p' is the <dst> projection and r' is some other position.
//
// For a transpose, T, we have:
//
//   r' = T.r
//
// so
//
//   dst[(p'.T).r] = src[p.r]
//
// but p'.T is just another projection.  In fact, if T swaps dimensions <i> and
// <j>, then p'.T swaps the i'th and j'th elements in p'.  By replacing p' with
// p'.T, the copy operation doesn't need to know anything about T itself; it
// just iterates over the domain of r using the respective projections to
// perform the mapping.
//
// In principle, this is true for any linear transform over the addressing
// vector space.
//
#ifdef __cplusplus
template<class Tdst, class Tsrc>
void
imCopyTranspose( Tdst *dst, size_t dst_pitch[4], 
                 Tsrc *src, size_t src_pitch[4], 
                 size_t shape[3], int i, int j )
{ int    ii = 1 + i,
         jj = 1 + j;
  size_t ti = dst_pitch[ii],
         tj = dst_pitch[jj];

  // Transpose by copying using permuted pitches.
  dst_pitch[ii] = tj;
  dst_pitch[jj] = ti;
  imCopy<Tdst,Tsrc>( dst, dst_pitch, src, src_pitch, shape );
  dst_pitch[ii] = ti;  // restore before returning
  dst_pitch[jj] = tj;

  // permute shape
  ti = shape[i];
  shape[i] = shape[j];
  shape[j] = ti;
}
#endif //#ifdef __cplusplus

#define DECL_IMCOPYTRANSPOSE(Tdstlbl,Tdst,Tsrclbl,Tsrc) \
  mydeclspec void imCopyTranspose_##Tdstlbl##_##Tsrclbl( Tdst *dst, size_t dst_pitch[4], Tsrc *src, size_t src_pitch[4], size_t n[3], int i, int j );

#define DECL_IMCOPYTRANSPOSE_BY_DEST(Tdstlbl,Tdst) \
  DECL_IMCOPYTRANSPOSE(Tdstlbl,Tdst ,u8 ,u8  );\
  DECL_IMCOPYTRANSPOSE(Tdstlbl,Tdst ,u16,u16 );\
  DECL_IMCOPYTRANSPOSE(Tdstlbl,Tdst ,u32,u32 );\
  DECL_IMCOPYTRANSPOSE(Tdstlbl,Tdst ,u64,u64 );\
  DECL_IMCOPYTRANSPOSE(Tdstlbl,Tdst ,i8 , i8  );\
  DECL_IMCOPYTRANSPOSE(Tdstlbl,Tdst ,i16, i16 );\
  DECL_IMCOPYTRANSPOSE(Tdstlbl,Tdst ,i32, i32 );\
  DECL_IMCOPYTRANSPOSE(Tdstlbl,Tdst ,i64, i64 );\
  DECL_IMCOPYTRANSPOSE(Tdstlbl,Tdst ,f32, f32   );\
  DECL_IMCOPYTRANSPOSE(Tdstlbl,Tdst ,f64, f64  );

DECL_IMCOPYTRANSPOSE_BY_DEST( u8 ,u8  );
DECL_IMCOPYTRANSPOSE_BY_DEST( u16,u16 );
DECL_IMCOPYTRANSPOSE_BY_DEST( u32,u32 );
DECL_IMCOPYTRANSPOSE_BY_DEST( u64,u64 );
DECL_IMCOPYTRANSPOSE_BY_DEST( i8 , i8  );
DECL_IMCOPYTRANSPOSE_BY_DEST( i16, i16 );
DECL_IMCOPYTRANSPOSE_BY_DEST( i32, i32 );
DECL_IMCOPYTRANSPOSE_BY_DEST( i64, i64 );
DECL_IMCOPYTRANSPOSE_BY_DEST( f32, f32   );
DECL_IMCOPYTRANSPOSE_BY_DEST( f64, f64  );
