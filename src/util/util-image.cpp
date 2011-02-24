
#include "util-image.h"

extern "C"
void Compute_Pitch( size_t pitch[4], size_t d2, size_t d1, size_t d0, size_t pixel_pitch)
{ pitch[3] = pixel_pitch;
  pitch[2] = d0 * pitch[3];
  pitch[1] = d1 * pitch[2];
  pitch[0] = d2 * pitch[1];
}

int _pitch_first_mismatch( size_t dst_pitch[4], size_t src_pitch[4] )
{ int i=4;
  //if( dst_pitch[i] != 1 || src_pitch[i] != 1 ) // There are two ways to interpret the lowest pitch, x
  //  return 3;                                  // 1. A pixel is x elements wide, copy all
 while(i--)                                      // 2. The smallest element is 1 element wide, and x tells you how many things to skip
    if( dst_pitch[i] != src_pitch[i] )           // The ambiguity is there because the <shape> argument doesn't say how many elements to copy
      return i;                                  // However, most of the time this can be taken care of by bluring how channels are defined.
  return i; // no mismatch...returns -1          // I think #1 is the more common default, so I'll stick with that.  The commented out region
}

#define DEFN_IMCASTCOPY(Tdstlbl,Tdst,Tsrclbl,Tsrc) \
  extern "C" void imCastCopy_##Tdstlbl##_##Tsrclbl( Tdst *dst, size_t dst_pitch[4], Tsrc *src, size_t src_pitch[4], size_t n[3] )\
  { imCastCopy<Tdst,Tsrc>(dst,dst_pitch,src,src_pitch,n); }

#define DEFN_IMCASTCOPY_BY_DEST(Tdstlbl,Tdst) \
  DEFN_IMCASTCOPY(Tdstlbl,Tdst ,u8 ,u8  );\
  DEFN_IMCASTCOPY(Tdstlbl,Tdst ,u16,u16 );\
  DEFN_IMCASTCOPY(Tdstlbl,Tdst ,u32,u32 );\
  DEFN_IMCASTCOPY(Tdstlbl,Tdst ,u64,u64 );\
  DEFN_IMCASTCOPY(Tdstlbl,Tdst ,i8 , i8  );\
  DEFN_IMCASTCOPY(Tdstlbl,Tdst ,i16, i16 );\
  DEFN_IMCASTCOPY(Tdstlbl,Tdst ,i32, i32 );\
  DEFN_IMCASTCOPY(Tdstlbl,Tdst ,i64, i64 );\
  DEFN_IMCASTCOPY(Tdstlbl,Tdst ,f32, f32   );\
  DEFN_IMCASTCOPY(Tdstlbl,Tdst ,f64, f64  );

DEFN_IMCASTCOPY_BY_DEST( u8 ,u8  );
DEFN_IMCASTCOPY_BY_DEST( u16,u16 );
DEFN_IMCASTCOPY_BY_DEST( u32,u32 );
DEFN_IMCASTCOPY_BY_DEST( u64,u64 );
DEFN_IMCASTCOPY_BY_DEST( i8 , i8  );
DEFN_IMCASTCOPY_BY_DEST( i16, i16 );
DEFN_IMCASTCOPY_BY_DEST( i32, i32 );
DEFN_IMCASTCOPY_BY_DEST( i64, i64 );
DEFN_IMCASTCOPY_BY_DEST( f32, f32   );
DEFN_IMCASTCOPY_BY_DEST( f64, f64  );


#define DEFN_IMCOPY(Tdstlbl,Tdst,Tsrclbl,Tsrc) \
  extern "C" void imCopy_##Tdstlbl##_##Tsrclbl( Tdst *dst, size_t dst_pitch[4], Tsrc *src, size_t src_pitch[4], size_t n[3] )\
  { imCopy<Tdst,Tsrc>(dst,dst_pitch,src,src_pitch,n); }

#define DEFN_IMCOPY_BY_DEST(Tdstlbl,Tdst) \
  DEFN_IMCOPY(Tdstlbl,Tdst ,u8 ,u8  );\
  DEFN_IMCOPY(Tdstlbl,Tdst ,u16,u16 );\
  DEFN_IMCOPY(Tdstlbl,Tdst ,u32,u32 );\
  DEFN_IMCOPY(Tdstlbl,Tdst ,u64,u64 );\
  DEFN_IMCOPY(Tdstlbl,Tdst ,i8 , i8  );\
  DEFN_IMCOPY(Tdstlbl,Tdst ,i16, i16 );\
  DEFN_IMCOPY(Tdstlbl,Tdst ,i32, i32 );\
  DEFN_IMCOPY(Tdstlbl,Tdst ,i64, i64 );\
  DEFN_IMCOPY(Tdstlbl,Tdst ,f32, f32   );\
  DEFN_IMCOPY(Tdstlbl,Tdst ,f64, f64  );

DEFN_IMCOPY_BY_DEST( u8 ,u8  );
DEFN_IMCOPY_BY_DEST( u16,u16 );
DEFN_IMCOPY_BY_DEST( u32,u32 );
DEFN_IMCOPY_BY_DEST( u64,u64 );
DEFN_IMCOPY_BY_DEST( i8 , i8  );
DEFN_IMCOPY_BY_DEST( i16, i16 );
DEFN_IMCOPY_BY_DEST( i32, i32 );
DEFN_IMCOPY_BY_DEST( i64, i64 );
DEFN_IMCOPY_BY_DEST( f32, f32   );
DEFN_IMCOPY_BY_DEST( f64, f64  );



#define DEFN_IMCOPYTRANSPOSE(Tdstlbl,Tdst,Tsrclbl,Tsrc) \
  extern "C" void imCopyTranspose_##Tdstlbl##_##Tsrclbl( Tdst *dst, size_t dst_pitch[4], Tsrc *src, size_t src_pitch[4], size_t n[3], int i, int j )\
  { imCopyTranspose<Tdst,Tsrc>(dst,dst_pitch,src,src_pitch,n,i,j); }

#define DEFN_IMCOPYTRANSPOSE_BY_DEST(Tdstlbl,Tdst) \
  DEFN_IMCOPYTRANSPOSE(Tdstlbl,Tdst ,u8 ,u8  );\
  DEFN_IMCOPYTRANSPOSE(Tdstlbl,Tdst ,u16,u16 );\
  DEFN_IMCOPYTRANSPOSE(Tdstlbl,Tdst ,u32,u32 );\
  DEFN_IMCOPYTRANSPOSE(Tdstlbl,Tdst ,u64,u64 );\
  DEFN_IMCOPYTRANSPOSE(Tdstlbl,Tdst ,i8 , i8  );\
  DEFN_IMCOPYTRANSPOSE(Tdstlbl,Tdst ,i16, i16 );\
  DEFN_IMCOPYTRANSPOSE(Tdstlbl,Tdst ,i32, i32 );\
  DEFN_IMCOPYTRANSPOSE(Tdstlbl,Tdst ,i64, i64 );\
  DEFN_IMCOPYTRANSPOSE(Tdstlbl,Tdst ,f32, f32   );\
  DEFN_IMCOPYTRANSPOSE(Tdstlbl,Tdst ,f64, f64  );

DEFN_IMCOPYTRANSPOSE_BY_DEST( u8 ,u8  );
DEFN_IMCOPYTRANSPOSE_BY_DEST( u16,u16 );
DEFN_IMCOPYTRANSPOSE_BY_DEST( u32,u32 );
DEFN_IMCOPYTRANSPOSE_BY_DEST( u64,u64 );
DEFN_IMCOPYTRANSPOSE_BY_DEST( i8 , i8  );
DEFN_IMCOPYTRANSPOSE_BY_DEST( i16, i16 );
DEFN_IMCOPYTRANSPOSE_BY_DEST( i32, i32 );
DEFN_IMCOPYTRANSPOSE_BY_DEST( i64, i64 );
DEFN_IMCOPYTRANSPOSE_BY_DEST( f32, f32   );
DEFN_IMCOPYTRANSPOSE_BY_DEST( f64, f64  );
