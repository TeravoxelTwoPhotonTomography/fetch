#include "stdafx.h"

void Compute_Pitch( size_t pitch[4], size_t d2, size_t d1, size_t d0, size_t pixel_pitch)
{ pitch[3] = pixel_pitch;
  pitch[2] = d0 * pitch[3];
  pitch[1] = d1 * pitch[2];
  pitch[0] = d2 * pitch[1];
}

int _pitch_first_mismatch( size_t dst_pitch[4], size_t src_pitch[4] )
{ int i = 3;
  //if( dst_pitch[i] != 1 || src_pitch[i] != 1 ) // There are two ways to interpret the lowest pitch, x
  //  return 3;                                  // 1. A pixel is x elements wide, copy all
  while(i--)                                     // 2. The smallest element is 1 element wide, and x tells you how many things to skip
    if( dst_pitch[i] != src_pitch[i] )           // The ambiguity is there because the <shape> argument doesn't say how many elements to copy
      return i;                                  // However, most of the time this can be taken care of by bluring how channels are defined.
  return i; // no mismatch...returns -1          // I think #1 is the more common default, so I'll stick with that.  The commented out region
}