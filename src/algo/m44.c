#include "m44.h"
#include <string.h>

#define mx(a,x) (*(a) = (*(a)<(x))?(x):*(a))
#define mn(a,x) (*(a) = ((x)<*(a))?(x):*(a))

void m3Ncolminmaxf(const float *m, int ncols, float *mins, float *maxs)
{ int i,j;

  for(i=0;i<3;++i)
  { mins[i] = m[ncols*i];
    maxs[i] = m[ncols*i]; 
  }
  for(i=0;i<3;++i)
  { const float *row = m + ncols*i;
    for(j=1;j<ncols;++j)
    { const float v = row[j];
      mx(maxs+i,v);
      mn(mins+i,v);
    }
  }
}

void m4Ncolminmaxf(const float *m, int ncols, float *mins, float *maxs)
{ int i,j;
  
  for(i=0;i<4;++i)
  { mins[i] = m[ncols*i];
    maxs[i] = m[ncols*i]; 
  }
  for(i=0;i<4;++i)
  { const float *row = m + ncols*i;
    for(j=1;j<ncols;++j)
    { const float v = row[j];
      mx(maxs+i,v);
      mn(mins+i,v);
    }
  }
}

void m44mulf(const float a[16], const float *b, int ncols, float *out)
{ int i,j,k;
  const float *rowa, *colb;
  float *rowdest;
  memset(out,0,sizeof(float)*4*ncols);
  for(i=0; i<4; i++)
  { rowa    = a   + i*4;
    rowdest = out + i*ncols; 
    for( j=0; j<4; j++ )
    { for( k=0; k<ncols; k++ )
      { colb = b + k;
        rowdest[k] += rowa[j] * colb[ncols*j];
      }
    }
  }
}

// b should be 3xN       - the (missing) 4th row is all ones 
// out is treated as 3xN
// a is assumed to have the block form [A b; 0 1]
void m44mulf_affine(const float a[16], const float *b, int ncols, float *out)
{ int i,j,k;
  const float *rowa, *colb;
  float *rowdest;
  memset(out,0,sizeof(float)*3*ncols);
  for(i=0; i<3; i++)                            // rows of a
  { rowa    = a   + i*4;
    rowdest = out + i*ncols; 
    for( j=0; j<3; j++ )                        // rows of b
    { for( k=0; k<ncols; k++ )                  // cols of b
      { colb = b + k;
        rowdest[k] += rowa[j] * colb[ncols*j];
      }
    }
    // the translation
    //  "virtual" last row of b is all 1's
    for( k=0; k<ncols; k++ )                  // cols of b
      rowdest[k] += rowa[j]; 
  }
}

int m44invf(const float m[16], float invOut[16])
{
float inv[16], det;
int i;

inv[0] =   m[5]*m[10]*m[15] - m[5]*m[11]*m[14] - m[9]*m[6]*m[15]
+ m[9]*m[7]*m[14] + m[13]*m[6]*m[11] - m[13]*m[7]*m[10];
inv[4] =  -m[4]*m[10]*m[15] + m[4]*m[11]*m[14] + m[8]*m[6]*m[15]
- m[8]*m[7]*m[14] - m[12]*m[6]*m[11] + m[12]*m[7]*m[10];
inv[8] =   m[4]*m[9]*m[15] - m[4]*m[11]*m[13] - m[8]*m[5]*m[15]
+ m[8]*m[7]*m[13] + m[12]*m[5]*m[11] - m[12]*m[7]*m[9];
inv[12] = -m[4]*m[9]*m[14] + m[4]*m[10]*m[13] + m[8]*m[5]*m[14]
- m[8]*m[6]*m[13] - m[12]*m[5]*m[10] + m[12]*m[6]*m[9];
inv[1] =  -m[1]*m[10]*m[15] + m[1]*m[11]*m[14] + m[9]*m[2]*m[15]
- m[9]*m[3]*m[14] - m[13]*m[2]*m[11] + m[13]*m[3]*m[10];
inv[5] =   m[0]*m[10]*m[15] - m[0]*m[11]*m[14] - m[8]*m[2]*m[15]
+ m[8]*m[3]*m[14] + m[12]*m[2]*m[11] - m[12]*m[3]*m[10];
inv[9] =  -m[0]*m[9]*m[15] + m[0]*m[11]*m[13] + m[8]*m[1]*m[15]
- m[8]*m[3]*m[13] - m[12]*m[1]*m[11] + m[12]*m[3]*m[9];
inv[13] =  m[0]*m[9]*m[14] - m[0]*m[10]*m[13] - m[8]*m[1]*m[14]
+ m[8]*m[2]*m[13] + m[12]*m[1]*m[10] - m[12]*m[2]*m[9];
inv[2] =   m[1]*m[6]*m[15] - m[1]*m[7]*m[14] - m[5]*m[2]*m[15]
+ m[5]*m[3]*m[14] + m[13]*m[2]*m[7] - m[13]*m[3]*m[6];
inv[6] =  -m[0]*m[6]*m[15] + m[0]*m[7]*m[14] + m[4]*m[2]*m[15]
- m[4]*m[3]*m[14] - m[12]*m[2]*m[7] + m[12]*m[3]*m[6];
inv[10] =  m[0]*m[5]*m[15] - m[0]*m[7]*m[13] - m[4]*m[1]*m[15]
+ m[4]*m[3]*m[13] + m[12]*m[1]*m[7] - m[12]*m[3]*m[5];
inv[14] = -m[0]*m[5]*m[14] + m[0]*m[6]*m[13] + m[4]*m[1]*m[14]
- m[4]*m[2]*m[13] - m[12]*m[1]*m[6] + m[12]*m[2]*m[5];
inv[3] =  -m[1]*m[6]*m[11] + m[1]*m[7]*m[10] + m[5]*m[2]*m[11]
- m[5]*m[3]*m[10] - m[9]*m[2]*m[7] + m[9]*m[3]*m[6];
inv[7] =   m[0]*m[6]*m[11] - m[0]*m[7]*m[10] - m[4]*m[2]*m[11]
+ m[4]*m[3]*m[10] + m[8]*m[2]*m[7] - m[8]*m[3]*m[6];
inv[11] = -m[0]*m[5]*m[11] + m[0]*m[7]*m[9] + m[4]*m[1]*m[11]
- m[4]*m[3]*m[9] - m[8]*m[1]*m[7] + m[8]*m[3]*m[5];
inv[15] =  m[0]*m[5]*m[10] - m[0]*m[6]*m[9] - m[4]*m[1]*m[10]
+ m[4]*m[2]*m[9] + m[8]*m[1]*m[6] - m[8]*m[2]*m[5];

det = m[0]*inv[0] + m[1]*inv[4] + m[2]*inv[8] + m[3]*inv[12];
if (det == 0)
        return 0;

det = 1.0f / det;

for (i = 0; i < 16; i++)
        invOut[i] = inv[i] * det;

return 1;
}

void m44f_copy(float *dst,const float *src)
{ memcpy(dst,src,sizeof(float)*16);
}

// performs A <- T*A
void m44f_translate_left(float *src, float dx, float dy, float dz)
{ float *t = src+3;
  t[0] += dx;
  t[4] += dy;
  t[8] += dz;
}

// performs A <- A*T
void m44f_translate_right(float *src, float dx, float dy, float dz)
{ float T[4] = {dx,dy,dz,1.0},AT[4];
  float *t = src+3;
  m44mulf_affine(src,T,1,AT);
  t[0] = AT[0];
  t[4] = AT[1];
  t[8] = AT[2];
}
