#include <stdio.h>
#include <math.h>

#include <array.h>
#include "config.h"
//#include "tictoc.h"

#include "cuda.h"
#include "cuda_runtime.h"
#include "device_launch_parameters.h"
#include "math_functions.h"

#define BLOCK_SIZE (128)
#define MAX_DEVICES (16)  //increase this if you've got more devices.

///// Imports from unwarp.c
extern "C" int  compute_map (float *xs, int w, float duty);
extern "C" void compute_norm(float *norm, float *xs, int w);

///// Error handling
#define REPORT_ERR(exprstr,ecode) \
  { if(ecode!=cudaSuccess)                       \
    { fprintf(stderr,                            \
        "**ERROR [CUDA Unwarp]"ENDL                     \
        "- %s(%d)"ENDL                           \
        "- Expression: %s"ENDL                   \
        "- \t%s "ENDL,                           \
        __FILE__,__LINE__,exprstr,cudaGetErrorString(e));\
      goto Error;                                \
    }                                            \
  }

#define REPORT_WRN(exprstr,ecode) \
  { if(ecode!=cudaSuccess)                       \
    { fprintf(stderr,                            \
        "**WARNING [CUDA Unwarp]"ENDL                   \
        "- %s(%d)"ENDL                           \
        "- Expression: %s"ENDL                   \
        "- \t%s "ENDL,                           \
        __FILE__,__LINE__,exprstr,cudaGetErrorString(e));\
    }                                            \
  }

#define ASRT(expr) \
  if(!(expr))                                    \
  { fprintf(stderr,                              \
      "**ERROR [Unwarp]\tAssertion failed"ENDL   \
      "- %s(%d): %s"ENDL                         \
      ,__FILE__,__LINE__,#expr);                 \
    goto Error;                                  \
  }


#define CHECK(expr) \
  { cudaError_t e = (expr);                      \
    REPORT_ERR(#expr,e);                         \
  }

#define WARN(expr) \
  { cudaError_t e = (expr);                      \
    REPORT_WRN(#expr,e);                         \
  }

#define EXPECT(expr,expect,lbl) \
{ cudaError e = (expr);         \
  if(e!=cudaSuccess)            \
  { if(e==expect) goto lbl;     \
    REPORT_ERR(#expr,e);        \
  }                             \
}

///// Utils
//static
//size_t bytesof(const Array *a)
//{ static const int Bpp[] = {1,2,3,4,1,2,3,4,4,8};
//  return a->size*Bpp[a->type];
//}
static
size_t bytesof_row(const Array *a)
{ static const int Bpp[] = {1,2,3,4,1,2,3,4,4,8};
  return a->dims[0]*Bpp[a->type];
}

///// row-wise lut kernel
// - Use 1D thread indexes.  Each addresses a row in the input data.
// - Each thread remaps the input data in a row to output pixels with averaging.
typedef unsigned int uint;

__device__ uint nearest(float x) { return rintf(x); }

// Lookup is by Nearest
// Composition is additive
template<typename Tin,typename Tout>
__global__ void rowwise_lut_kernel(
  const Tin*    __restrict__  in, 
        Tout*   __restrict__  out,
  const uint                  width_in,
  const uint                  width_out,
  const float* __restrict__   lut,
  const float* __restrict__   norm,
  const uint                  nrows)
{   
  const uint  irow = threadIdx.x + blockIdx.x*blockDim.x;
  if(irow>=nrows) return; // bounds check

  const Tin*   rin = in + width_in * irow;
        Tout* rout = out+ width_out* irow;

  for(uint i=0;i<width_in;++i)
  { uint j = nearest(lut[i]);
    rout[j] += rin[i]/norm[j];
  }
}

#define KERNELCALL(t1,t2) rowwise_lut_kernel<t1,t2><<< grid,threads,0,streams[i] >>>((t1*)dev_ins[i],(t2*)dev_outs[i],width_in,width_out,dev_luts[i],dev_norms[i],nrows)
#define KERNELCALLS(t1) \
  switch(out->type)                                         \
  {                                                         \
    case UINT8_TYPE:    KERNELCALL(t1,uint8 ); break;       \
    case UINT16_TYPE:   KERNELCALL(t1,uint16); break;       \
    case UINT32_TYPE:   KERNELCALL(t1,uint32); break;       \
    case UINT64_TYPE:   KERNELCALL(t1,uint64); break;       \
    case  INT8_TYPE:    KERNELCALL(t1, int8 ); break;       \
    case  INT16_TYPE:   KERNELCALL(t1, int16); break;       \
    case  INT32_TYPE:   KERNELCALL(t1, int32); break;       \
    case  INT64_TYPE:   KERNELCALL(t1, int64); break;       \
    case  FLOAT32_TYPE: KERNELCALL(t1, float); break;       \
    case  FLOAT64_TYPE: KERNELCALL(t1, double); break;      \
    default: goto Error;                                    \
  }

extern "C" int unwarp_gpu(Array *out, Array *in, float duty)
{   
  cudaStream_t streams[MAX_DEVICES];
  void  *dev_ins[MAX_DEVICES],  *dev_outs[MAX_DEVICES];
  float *dev_luts[MAX_DEVICES], *dev_norms[MAX_DEVICES];
  float *xs,*norm;
  const int w = in->dims[0];
  const size_t bytesof_lut = sizeof(float)*w;

  // Compute the LUT
  ASRT( xs=(float*)malloc(2*bytesof_lut) ); // alloc for xs and norm
  norm = xs + w;
  compute_map(xs,w,duty);
  compute_norm(norm,xs,w);


  // feed the devices
  { int totalrows = in->dims[1]*in->dims[2];
    int ndevices;
    CHECK( cudaGetDeviceCount(&ndevices) );
    ASRT( ndevices < MAX_DEVICES );  
    // init streams
    for(int i=0;i<ndevices;++i)
    {
      cudaSetDevice(i);
      CHECK( cudaStreamCreate(streams+i));
    }
    // alloc for devices
    for(int i=0;i<ndevices;++i)
    { cudaSetDevice(i);      
      uint nrows = totalrows/ndevices,
           irow  = nrows*i;
      if(i==ndevices-1)           // take care of any rounding problems on the last iteration
        nrows = totalrows - irow;    

      CHECK( cudaMalloc(&dev_ins[i]  ,nrows*bytesof_row(in)) );
      CHECK( cudaMalloc(&dev_outs[i] ,nrows*bytesof_row(out)) );
      CHECK( cudaMalloc(&dev_luts[i] ,bytesof_lut) );
      CHECK( cudaMalloc(&dev_norms[i],bytesof_lut) );
      CHECK( cudaMemset(dev_outs[i],0,nrows*bytesof_row(out)) );
    }
    // upload data
    for(int i=0;i<ndevices;++i)
    { cudaSetDevice(i);      
      uint nrows = totalrows/ndevices,
           irow  = nrows*i;
      if(i==ndevices-1)           // take care of any rounding problems on the last iteration
        nrows = totalrows - irow;
      CHECK( cudaMemcpyAsync(dev_ins[i],  
                             AUINT8(in) + irow*bytesof_row(in), 
                             nrows*bytesof_row(in),
                             cudaMemcpyHostToDevice,
                             streams[i]));  
      CHECK( cudaMemcpyAsync(dev_luts[i], xs,   bytesof_lut,cudaMemcpyHostToDevice,streams[i]) );  
      CHECK( cudaMemcpyAsync(dev_norms[i],norm, bytesof_lut,cudaMemcpyHostToDevice,streams[i]) );  
    }
    // execute
    for(int i=0;i<ndevices;++i)
    { cudaSetDevice(i);      
      uint nrows = totalrows/ndevices,
           irow  = nrows*i;
      if(i==ndevices-1)           // take care of any rounding problems on the last iteration
        nrows = totalrows - irow;
    
      uint width_in  =  in->dims[0],
           width_out = out->dims[0];
      dim3 threads(BLOCK_SIZE,1,1),
           grid((unsigned int)ceil(nrows/(float)BLOCK_SIZE),1,1);

      switch(in->type)
      {                                                     
        case   UINT8_TYPE: KERNELCALLS(uint8 ) break;   
        case  UINT16_TYPE: KERNELCALLS(uint16) break;   
        case  UINT32_TYPE: KERNELCALLS(uint32) break;   
        case  UINT64_TYPE: KERNELCALLS(uint64) break;   
        case    INT8_TYPE: KERNELCALLS( int8 ) break;   
        case   INT16_TYPE: KERNELCALLS( int16) break;   
        case   INT32_TYPE: KERNELCALLS( int32) break;   
        case   INT64_TYPE: KERNELCALLS( int64) break;   
        case FLOAT32_TYPE: KERNELCALLS( float) break; 
        case FLOAT64_TYPE: KERNELCALLS(double) break;
        default:
          goto Error;
      }
    }
    // download results
    for(int i=0;i<ndevices;++i)  
    { cudaSetDevice(i);      
      uint nrows = totalrows/ndevices,
           irow  = nrows*i;
      if(i==ndevices-1)           // take care of any rounding problems on the last iteration
        nrows = totalrows - irow;
      CHECK(cudaMemcpyAsync(AUINT8(out) + irow*bytesof_row(out),
                            dev_outs[i],
                            nrows*bytesof_row(out),
                            cudaMemcpyDeviceToHost,
                            streams[i]));
    }
    // cleanup
    for(int i=0;i<ndevices;++i)
    { cudaSetDevice(i);
      CHECK(cudaStreamSynchronize(streams[i]));
      CHECK( cudaStreamDestroy(streams[i]) );
      CHECK( cudaFree(dev_ins[i]) );
      CHECK( cudaFree(dev_outs[i]) );
      CHECK( cudaFree(dev_luts[i]) );
      CHECK( cudaFree(dev_norms[i]) );
    } 
  }
    
  free(xs);
  
  return 1;
Error:
  return 0;
}
