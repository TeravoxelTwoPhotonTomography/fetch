#pragma once
#include <string.h>
#include <stdio.h>

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

void Compute_Pitch( size_t pitch[4], size_t d2, size_t d1, size_t d0, size_t pixel_pitch);

int _pitch_first_mismatch( size_t dst_pitch[4], size_t src_pitch[4] );

// cast and copy - always uses a slow copy to gaurantee casting
template<class Tdst,class Tsrc>
void
imCastCopy( Tdst *dst, size_t dst_pitch[4], Tsrc *src, size_t src_pitch[4], size_t n[3] )
{ size_t i = n[0], 
         b = sizeof(Tsrc),
         vols[3] = { n[2]*n[1]*n[0]*b,  // in bytes.  used in block copies
                          n[1]*n[0]*b, 
                               n[0]*b},
          v  = vols[2],
          dp0 = dst_pitch[1], // the biggest jump
          dp1 = dst_pitch[2],
          dp2 = dst_pitch[3], // the smallest jump
          sp0 = src_pitch[1],
          sp1 = src_pitch[2],
          sp2 = src_pitch[3];
    while(i--)
    { size_t j = n[1];
      Tdst *dc0 = dst + dp0 * i;
      Tsrc *sc0 = src + sp0 * i;
      while(j--)
      { size_t k = n[2]; 
        Tdst *dc1 = dc0 + dp1 * j;
        Tsrc *sc1 = sc0 + sp1 * j;
        while(k--)
        { Tdst *dc2 = dc1 + dp2 * k;
          Tsrc *sc2 = sc1 + sp2 * k;
          *dc2 = (Tdst) (*sc2);       // Cast
        }
      }
    }
}


// copies - casting not gauranteed
template<class Tdst, class Tsrc>
void
imCopy( Tdst *dst, size_t dst_pitch[4], Tsrc *src, size_t src_pitch[4], size_t n[3] )
{ int mismatch = _pitch_first_mismatch(dst_pitch, src_pitch);
  size_t b = sizeof(Tsrc),
         vols[3] = { n[0]*n[1]*n[2]*b,  // in bytes.  used in block copies
                          n[1]*n[2]*b, 
                               n[2]*b};
  if( sizeof(Tdst) != sizeof(Tsrc) )
    mismatch = 3; //a mismatch on the pixel level will force the pixel by pixel copy

  switch( mismatch )
  { case -1: // all strides match
    case  0: // all but top level strides match - that is, only the total size differs
      memcpy( dst, src, vols[0] );
      break;
    case  1:                        // "planes" as chunks
      { size_t i = n[0],            // number of planes
               v = vols[1],         // number of elements in a chunk
               dp = dst_pitch[1],   // plane pitch
               sp = src_pitch[1];   //
        while(i--)
        { Tdst *dc = dst + dp * i;
          Tsrc *sc = src + sp * i;
          memcpy( dc, sc, v );
        }
      }
      break;
    case  2:                        // "line" as chunks
      { size_t i = n[0],            // number of planes
               v   = vols[2],       // number of elements in a chunk
               dp0 = dst_pitch[1],  // plane pitch
               dp1 = dst_pitch[2],  // line pitch
               sp0 = src_pitch[1],  // plane pitch
               sp1 = src_pitch[2];  // line pitch
        while(i--)
        { size_t j = n[1];          // number of lines
          Tdst *dc0 = dst + dp0 * i;// plane addresses
          Tsrc *sc0 = src + sp0 * i;
          while(j--)
          { Tdst *dc1 = dc0 + dp1 * j;// line addresses
            Tsrc *sc1 = sc0 + sp1 * j;
            memcpy( dc1, sc1, v );
          }
        }
      }
      break;
    case  3:                        // pixel copy
    default:
      { size_t i = n[0],            // number of planes
               dp0 = dst_pitch[1],  // plane pitch
               dp1 = dst_pitch[2],  // line  pitch
               dp2 = dst_pitch[3],  // pixel pitch
               sp0 = src_pitch[1],
               sp1 = src_pitch[2],
               sp2 = src_pitch[3];
        while(i--)
        { size_t j = n[1];                // number of lines
          Tdst *dc0 = dst + dp0 * i;      // plane address
          Tsrc *sc0 = src + sp0 * i;
          while(j--)
          { size_t k = n[2];              // number of pixels
            Tdst *dc1 = dc0 + dp1 * j;    // line address
            Tsrc *sc1 = sc0 + sp1 * j;
            while(k--)
            { Tdst *dc2 = dc1 + dp2 * k;  // pixel address
              Tsrc *sc2 = sc1 + sp2 * k;
              *dc2 = (Tdst) (*sc2);       // Cast
            }
          }
        }
      }
      break;
  }
}



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

