#pragma warning(disable:4244) // conversion, possible loss of data

#include "unwarp.h"
#include <string.h>
#define _USE_MATH_DEFINES
#include <math.h>
#include "config.h"

#define ASRT(expr) \
  if(!(expr))                                    \
  { fprintf(stderr,                              \
      "**ERROR [Cosine unwarp] Assertion failed"ENDL \
      "- %s(%d) %s"ENDL                          \
      ,__FILE__,__LINE__,#expr);                 \
    goto Error;                                  \
  }

static
float map(int t, int inwidth, float duty)
{ const float d2 = 1.0f - 2.0f*(1.0f-duty), //fraction of half period per line
          domain = inwidth/d2,              // e.g. 878/0.9 = 975.56
            xoff = inwidth,                 // point of zero phase (right hand side of input image)
               k = M_PI/domain,
        prelut_i = 0.5f*(1.0f-cos(k*(t-xoff))),
            yoff = 0.5f*(1.0f-cos(k*( -xoff))); // prelut(0) 
  return 1.0f-(yoff-prelut_i)/yoff;             // domain:[0,inwidth-1] --> range: [0,1] // HACK? hardcoded flip
}


static
void mx(float *arg, float v)
{ *arg = (v>*arg)?v:*arg; 
}

static
size_t bytesof(const Array *a)
{ static const int Bpp[] = {1,2,3,4,1,2,3,4,4,8};
  return a->size*Bpp[a->type];
}

static int nearest(float x) 
{ return floorf(x+0.5f); //round 
}

int compute_map(float *xs, int w, float duty)
{
  int wout;
  // compute the warp to the unit interval
  { int i;
    for(i=0;i<w;++i)
      xs[i] = map(i,w,duty);
  }
  // Find the biggest difference between consecutive samples
  // Use that to determine the output image width.  
  // Choose s.t. max between samples is 1 px.  
  { float dx = 0.0;
    int i;
    for(i=1;i<w;++i)
      mx(&dx,fabs(xs[i]-xs[i-1]));
    wout = (Dimn_Type)(1.0f/dx);
  }
  // Scale to output width [0,1]->[0,wout-0.5]
  { int i;
    for(i=0;i<w;++i)
      xs[i] *= (wout-0.5001f);
  }
  return wout;
}

void compute_norm(float *norm, float *xs, int w)
{ int i;
  memset(norm,0,sizeof(float)*w);
  for(i=0;i<w;++i)
    norm[nearest(xs[i])]++;
}

void unwarp_get_dims(Dimn_Type *dims, Array *in, float duty)
{ 
  memcpy(dims,in->dims,sizeof(Dimn_Type)*in->ndims);
  unwarp_get_dims_ip(dims,duty);
}

void unwarp_get_dims_ip(Dimn_Type *dims, float duty)
{ 
  int w = (int) dims[0];
  { float dx = 0.0;
    int i;
    for(i=1;i<w;++i)
      mx(&dx,fabs(map(i,w,duty)-map(i-1,w,duty)));
    dims[0] = (Dimn_Type)(1.0f/dx);
  }
}


#define UNWARPLOOP(Tin,Tout) \
    for(iz=0;iz<in->dims[2];++iz)                                      \
    { Tin  *iplane = ((Tin*)  in->data) + countof_plane_in *iz;        \
      Tout *oplane = ((Tout*)out->data) + countof_plane_out*iz;        \
      for(iy=0;iy<in->dims[1];++iy)                                    \
      { Tin  *irow = iplane + countof_row_in *iy;                      \
        Tout *orow = oplane + countof_row_out*iy;                      \
        for(i=0;i<w;++i)                                               \
        { int j = nearest(xs[i]);                                      \
          orow[j] += irow[i]/norm[j]; /*norm here to avoid overflows*/ \
        }                                                              \
      }                                                                \
    }

#define UNWARP_LOOPS(Tout) \
  switch(in->type)                                           \
  {                                                          \
    case   UINT8_TYPE: UNWARPLOOP(uint8  ,Tout);break;       \
    case  UINT16_TYPE: UNWARPLOOP(uint16 ,Tout);break;       \
    case  UINT32_TYPE: UNWARPLOOP(uint32 ,Tout);break;       \
    case  UINT64_TYPE: UNWARPLOOP(uint64 ,Tout);break;       \
    case    INT8_TYPE: UNWARPLOOP(int8   ,Tout);break;       \
    case   INT16_TYPE: UNWARPLOOP(int16  ,Tout);break;       \
    case   INT32_TYPE: UNWARPLOOP(int32  ,Tout);break;       \
    case   INT64_TYPE: UNWARPLOOP(int64  ,Tout);break;       \
    case FLOAT32_TYPE: UNWARPLOOP(float  ,Tout);break;       \
    case FLOAT64_TYPE: UNWARPLOOP(double ,Tout);break;       \
    default:                                                 \
      return 0;                                              \
  }


int unwarp_cpu(Array* out, Array* in, float duty)
{ float *xs,*norm;
  int w = in->dims[0];
  memset(out->data,0,bytesof(out));
  ASRT( xs=(float*)malloc(2*sizeof(float)*w) ); // alloc for xs and norm
  norm = xs + w;
  compute_map(xs,w,duty);
  compute_norm(norm,xs,w);
  { Dimn_Type iy,iz,i;
    size_t countof_plane_in  =  in->dims[0]* in->dims[1],
           countof_row_in    =  in->dims[0],
           countof_plane_out = out->dims[0]*out->dims[1],
           countof_row_out   = out->dims[0];
    switch(out->type)
    { 
      case   UINT8_TYPE: UNWARP_LOOPS(uint8);  break;
      case  UINT16_TYPE: UNWARP_LOOPS(uint16); break;
      case  UINT32_TYPE: UNWARP_LOOPS(uint32); break;
      case  UINT64_TYPE: UNWARP_LOOPS(uint64); break;
      case    INT8_TYPE: UNWARP_LOOPS(int8);   break;
      case   INT16_TYPE: UNWARP_LOOPS(int16);  break;
      case   INT32_TYPE: UNWARP_LOOPS(int32);  break;
      case   INT64_TYPE: UNWARP_LOOPS(int64);  break;
      case FLOAT32_TYPE: UNWARP_LOOPS(float);  break;
      case FLOAT64_TYPE: UNWARP_LOOPS(double); break;
      default:
        return 0;
    }  
  }
        
  free(xs);
  return 1;
Error:
  return 0;
}

