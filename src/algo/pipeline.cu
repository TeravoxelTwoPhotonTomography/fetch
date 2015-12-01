#include "pipeline.h"
#include "pipeline-image.h"
#define _USE_MATH_DEFINES
#include <math.h>
#include <stdio.h>  //for printf
#include <stdlib.h> //for malloc
#include <string.h> //for memset
#include <stdint.h>

#include "cuda_runtime.h"

#define BX_   (32)
#define BY_   (4)
#define WORK_ (8)

#if 0
#define ECHO(estr)   LOG("---\t%s\n",estr)
#else
#define ECHO(estr)
#endif
static void breakme() {}

#define LOG(...)     printf(__VA_ARGS__)
#define REPORT(estr,msg) LOG("%s(%d): %s()\n\t%s\n\t%s\n",__FILE__,__LINE__,__FUNCTION__,estr,msg)
#define TRY(e)       do{ECHO(#e);if(!(e)){REPORT(#e,"Evaluated to false.");breakme(); goto Error;}}while(0)
#define FAIL(msg)    do{REPORT("Failure.",msg);goto Error;} while(0)
#define NEW(T,e,N)   TRY((e)=(T*)malloc(sizeof(T)*(N)))
#define ZERO(T,e,N)  memset((e),0,sizeof(T)*(N))

#define CUREPORT(ecode,estr) LOG("%s(%d): %s()\n\t%s\n\t%s\n",__FILE__,__LINE__,__FUNCTION__,estr,cudaGetErrorString(ecode))
#define CUTRY(e)             do{ cudaError_t ecode; ECHO(#e); ecode=(e); if(ecode!=cudaSuccess){CUREPORT(ecode,#e);goto Error;}}while(0)
#define CUWARN(e)            do{ cudaError_t ecode; ECHO(#e); ecode=(e); if(ecode!=cudaSuccess){CUREPORT(ecode,#e);           }}while(0)
#define CUNEW(T,e,N)    CUTRY(cudaMalloc((void**)&(e),sizeof(T)*(N)))
#define CUZERO(T,e,N)   CUTRY(cudaMemset((e),0,sizeof(T)*(N)))

#define countof(e)   (sizeof(e)/sizeof(*(e)))

#define CEIL(num,den) (((num)+(den)-(1))/(den))
/**
 * The parameter collection that gets passed to the kernel
 */
struct pipeline_ctx_t
{ unsigned * __restrict__ ilut;        ///< look up table for unwarp (ctx.w/2 number of elements).  Set by launch on first call or if width changes.
  float    * __restrict__ lut_norms0,
           * __restrict__ lut_norms1;
  unsigned istride,     ///< number of elements between rows of source.
           ostride;     ///< number of elements between rows of output.
  unsigned w,h;         ///< source width and height (height is nrows*nchan)
};

/**
 * The object that manages pipeline execution.
 */
typedef struct pipeline_t_
{ pipeline_ctx_t ctx;
  unsigned       count, ///< the number of frames that have been pushed to the accumulator
                 every; ///< the number of frames to average
  double         samples_per_scan;
  bool           invert;
  unsigned       downsample;
  unsigned       alignment;         ///< output rows are aligned to this target number of elements.
  unsigned       nbytes_tmp;
  float    norm,        ///< 1.0/the frame count as a float - set by launcher (eg. for ctx.every=4, this should be 0.25)
           m,b;         ///< slope and intercept for intensity scaling
  void  * __restrict__ src,         ///< device buffer
        * __restrict__ dst;         ///< device buffer
  float * __restrict__ tmp;         ///< device buffer
} *pipeline_t;

//
// --- KERNELS ---
//

// should schedule for destination (2*width/WORK/BX,height/2/BY) blocks [eg for 4864x512 -> (38,8)]
// dst width must be aligned  to WORK*BX   (256)
//     height must be aligned to BY*2      (64)
// ilut must be aligned to WORK*BX and sized 2*dst width
//
template<typename T,    ///< pixel type (input and output)
         unsigned BX,   ///< block size in X
         unsigned BY,   ///< block size in Y (channel dimension unrolled into Y)
         unsigned WORK  ///< number of elements to process per thread
         >
__global__ void __launch_bounds__(BX*BY,1) /* max threads, min blocks */
warp_kernel(pipeline_ctx_t ctx, const T* __restrict__ src, float* __restrict__ dst)
{ const unsigned ox=threadIdx.x+blockIdx.x*WORK*BX,
                 oy=threadIdx.y+blockIdx.y*BY;
  unsigned * __restrict__ ilut = ctx.ilut + ox;
  dst+=ox+oy*ctx.ostride*2;
  src+=   oy*ctx.istride;
  if(blockIdx.x<ctx.ostride/(WORK*BX)) // forward scan
  {
#pragma unroll
    for(int i=0;i<WORK;++i)
    { const int j0=ilut[i*BX],
                j1=ilut[i*BX+1];
      float v0=0.0f,v1=0.0f;
      if(j0>0) v1=ctx.lut_norms1[j0-1]*src[j0-1];
      for(int j=j0;j<j1;++j)
        v0+=ctx.lut_norms0[j]*src[j];
      dst[i*BX]+=v0+v1;
    }
  } else { // backward scan
#pragma unroll
    for(int i=0;i<WORK;++i)
    { const int j1=ilut[i*BX+1],
                j0=ilut[i*BX+2];
      float v0=0.0f;
      for(int j=j0;j<j1;++j)
        v0+=ctx.lut_norms0[j]*src[j];
      float v1=ctx.lut_norms1[j1]*src[j1];
      dst[i*BX]+=v0+v1;
    }
  }
}

/**
 * Cast array from float to T.
 * Rounds pixel values, so this isn't appropriate for converting to floating point types.
 * src and dst should be the same shape but may be different types.
 * Both must have width aligned to BX*WORK
 * Both must have heigh aligned to BY
 */
template<typename T,    ///< pixel type (input and output)
         unsigned BX,   ///< block size in X
         unsigned BY,   ///< block size in Y (channel dimension unrolled into Y)
         unsigned WORK  ///< number of elements to process per thread
         >
__global__ void __launch_bounds__(BX*BY,1)
cast_kernel(T*__restrict__ dst, const float* __restrict__ src, unsigned stride,const float m, const float b)
{ const int ox=threadIdx.x+blockIdx.x*WORK*BX,
            oy=threadIdx.y+blockIdx.y*BY;
  //if(oy>=h) return; // for unaligned y, uncomment and add an argument to the kernel call
  src+=ox+oy*stride;
  dst+=ox+oy*stride;
  #pragma unroll
  for(int i=0;i<WORK;++i)
    dst[i*BX]=round(fmaf(src[i*BX],m,b));
}

//
// --- PUBLIC INTERFACE ---
//

pipeline_t pipeline_make(const pipeline_param_t *params)
{ pipeline_t self=NULL;
  TRY(params);
  NEW(pipeline_t_,self,1);
  ZERO(pipeline_t_,self,1);
  self->every            = (params->frame_average_count<1)?1:params->frame_average_count;
  self->samples_per_scan = params->sample_rate_MHz*1.0e6/(double)params->scan_rate_Hz;
  self->invert           = (params->invert_intensity!=0);
  self->downsample       = (params->pixel_average_count<=1)?1:params->pixel_average_count;
  self->alignment        = BX_*WORK_;
  self->norm             = 1.0f/(float)self->every;
  self->m                = 1.0f;;
  self->b                = 0.0f;
  return self;
Error:
  return NULL;
}

void pipeline_free(pipeline_t *self)
{ if(self && *self)
  { void *ptrs[]={self[0]->ctx.ilut,
                  self[0]->ctx.lut_norms0,
                  self[0]->src,
                  self[0]->dst,
                  self[0]->tmp};
    for(int i=0;i<countof(ptrs);++i)
      if(ptrs[i])
        CUWARN(cudaFree(ptrs[i]));
    free(*self); *self=NULL;
  }
}

#define EPS (1e-3)
static unsigned pipeline_get_output_width(pipeline_t self, const double inwidth)
{ const double d=1.0-inwidth/self->samples_per_scan; // 1 - duty
  //max derivative of the cosine warp adjusted to cos(2pi*(d/2)) is the zero point
  //and the positive part of the warp function goes from 0 to 1.
  const double maxslope=M_PI*(1.0-d)/inwidth/cos(M_PI*d);
  const double amplitude=1.0/maxslope;
  const unsigned w=self->alignment*(unsigned)(amplitude/self->downsample/self->alignment);
  TRY(-EPS<d && d<=(0.5+EPS));
  TRY(0<w && w<inwidth);
  return w;
Error:
  return 0;
}
#undef EPS

extern "C" int pipeline_get_output_dims(pipeline_t self, const pipeline_image_t src, unsigned *w, unsigned *h, unsigned *nchan)
{ TRY(self && src);
  if(nchan) *nchan=src->nchan;
  if(h)     *h=src->h*2;
  if(w)     TRY(*w=pipeline_get_output_width(self,src->w));
  return 1;
Error:
  return 0;
}

static int pipeline_alloc_lut(pipeline_t self, unsigned inwidth)
{ unsigned N=self->alignment*CEIL(inwidth,self->alignment); // pad to aligned width
  if(self->ctx.ilut)
  { CUTRY(cudaFree(self->ctx.ilut));
    CUTRY(cudaFree(self->ctx.lut_norms0));
  }
  const unsigned     ow = pipeline_get_output_width(self,inwidth);
  CUNEW(unsigned,self->ctx.ilut,      2*(ow+1));
  CUNEW(float   ,self->ctx.lut_norms0,2*N+1);
  return 1;
Error:
  self->ctx.ilut=NULL;
  self->ctx.lut_norms0=NULL;
  return 0;

}

static void dump(const char* name, void* data, size_t nbytes)
{ FILE* fp=0;
  TRY(fp=fopen(name,"wb"));
  fwrite(data,1,nbytes,fp);
  fclose(fp);
Error:;
}

static double f(double x) { return  0.5*(1.0-cos(2.0*M_PI*x)); }

static int pipeline_fill_lut(pipeline_t self, unsigned inwidth)
{ int isok=1;
  unsigned * __restrict__ lut=0;
  unsigned * __restrict__ ilut=0;
  float    * __restrict__ norms=0;
  // useful constants
  const double        d = (1.0-inwidth/self->samples_per_scan)/2.0; // 0.5*(1 - duty)
  const unsigned     ow = pipeline_get_output_width(self,inwidth);
  const double        s = (1.0-2.0*d)/(double)inwidth;
  const double        A = ow/(1.0-f(d));
  const double      Afd = A*f(d);
  const unsigned  halfw = inwidth/2;
  const unsigned      N = self->alignment*CEIL(inwidth,self->alignment); // pad to aligned width
  // alloc temporary space
  NEW(unsigned ,lut  ,inwidth);
  NEW(unsigned ,ilut ,2*(ow+1));
  NEW(float    ,norms,2*N+1);
  ZERO(unsigned,lut  ,inwidth);
  ZERO(unsigned,ilut ,2*(ow+1));
  ZERO(float   ,norms,2*N+1);

  // compute lookup
  for(unsigned i=0;i<inwidth;++i)
  { double p0=d+s*i,
           p1=d+s*(i+1);
    double v0=A*f(p0)-Afd,
           v1=A*f(p1)-Afd;

    int j,k;
    if(v0<0.0) v0=0.0;
    if(v1<0.0) v1=0.0;
    if(v0>v1) { double v=v0;v0=v1;v1=v; } //swap
    j = (int) v0;
    k = (int) v1;
    TRY( (k-j)<2 ); // longest length should be 1, so shouldn't straddle more than two pixels
    lut[i] = j + (i<halfw?0:ow);
    if( (k-j)==0 )
    { norms[i]   = v1-v0;
      norms[i+N] = 0.0;
    } else { //k-j==1 -> k=1+j
      norms[i]   = k-v0;
      norms[i+N] = v1-k;
    }
  }

  // interval encode lookup table on output side
  { unsigned last=0;
    for(unsigned i=0;i<halfw;++i)
      if(last!=lut[i])
        ilut[last=lut[i]]=i;
    ilut[ow  ]=inwidth/2; // add elements to deal with discontinuity
    ilut[ow+1]=inwidth; // subtract one to prevent reading off end
    ilut+=2;
    for(unsigned i=halfw;i<inwidth;++i)
      if(last!=lut[i])
        ilut[(last=lut[i])]=i;
    ilut-=2;
  }
#if 0
  dump("lut.u32",lut    ,inwidth*sizeof(*lut));
  dump("norms.f32",norms,2*N    *sizeof(*norms));
  dump("ilut.u32",ilut  ,2*(ow+1) *sizeof(*ilut));
#endif

  // upload
  CUTRY(cudaMemcpy(self->ctx.ilut      ,ilut , 2*(ow+1)*sizeof(*ilut),cudaMemcpyHostToDevice));
  CUTRY(cudaMemcpy(self->ctx.lut_norms0,norms,(2*N+1)*sizeof(*norms) ,cudaMemcpyHostToDevice));
  self->ctx.lut_norms1=self->ctx.lut_norms0+N;

Finalize:
  if(lut)   free(lut);
  if(ilut)  free(ilut);
  if(norms) free(norms);
  return isok;
Error:
  isok=0;
  goto Finalize;
}

static int pipeline_upload(pipeline_t self, pipeline_image_t dst, const pipeline_image_t src)
{ if(self->src && (self->ctx.w!=src->w || self->ctx.h!=src->h*src->nchan)) // if there's a shape change, realloc
  { CUTRY(cudaFree(self->src)); self->src=0;
    CUTRY(cudaFree(self->dst)); self->dst=0;
    CUTRY(cudaFree(self->tmp)); self->tmp=0;
  }
  dst->h++; // pad by a line
  if(!self->src)
  { CUTRY(cudaMalloc((void**)&self->src,pipeline_image_nbytes(src)+1024));
    CUTRY(cudaMalloc((void**)&self->dst,pipeline_image_nbytes(dst)));
    CUTRY(cudaMalloc((void**)&self->tmp,self->nbytes_tmp=pipeline_image_nelem(dst)*sizeof(float)));
    CUTRY(cudaMemset(self->tmp,0,pipeline_image_nelem(dst)*sizeof(float)));
  }
  CUTRY(cudaMemcpy(self->src,src->data,pipeline_image_nbytes(src),cudaMemcpyHostToDevice));
  dst->h--; // restore original number of lines
  self->ctx.w=src->w;
  self->ctx.h=src->h*src->nchan;
  self->ctx.istride=src->stride;
  self->ctx.ostride=dst->stride;
  return 1;
Error:
  return 0;
}

static int pipeline_download(pipeline_t self, pipeline_image_t dst)
{ TRY(self->dst);
  CUTRY(cudaMemcpy(dst->data,self->dst,pipeline_image_nbytes(dst),cudaMemcpyDeviceToHost));
  return 1;
Error:
  return 0;
}


template<typename Tsrc, typename Tdst,unsigned BX,unsigned BY,unsigned WORK>
static int launch(pipeline_t self, int *emit)
{ unsigned ow=pipeline_get_output_width(self,self->ctx.w);
  dim3 threads(BX,BY),
       blocks(CEIL(2*ow,BX*WORK),CEIL(self->ctx.h,BY));      // for the cast from tmp to dst
  TRY(emit);
#if 1
  if(self->every>1) // frame averaging enabled
  { if( ((self->count+1)%self->every)==0 )
    { *emit=1;
      warp_kernel<Tsrc,BX,BY,WORK><<<blocks,threads>>>(self->ctx,(Tsrc*)self->src,self->tmp);
      cast_kernel<Tdst,BX,BY,WORK><<<blocks,threads>>>((Tdst*)self->dst,self->tmp,self->ctx.ostride*2,self->m*self->norm,self->b);
    } else
    { *emit=0;
      warp_kernel<Tsrc,BX,BY,WORK><<<blocks,threads>>>(self->ctx,(Tsrc*)self->src,self->tmp);
    }
    self->count++;
  } else            // frame averaging disabled
#endif
  { if(emit) *emit=1;
    warp_kernel<Tsrc,BX,BY,WORK><<<blocks,threads>>>(self->ctx,(Tsrc*)self->src,self->tmp);
    cast_kernel<Tdst,BX,BY,WORK><<<blocks,threads>>>((Tdst*)self->dst,self->tmp,self->ctx.ostride*2,self->m,self->b);
    CUTRY(cudaGetLastError());
  }
  if(*emit)
    CUTRY(cudaMemset(self->tmp,0,self->nbytes_tmp));
  return 1;
Error:
  return 0;
}

// generics

/** Requires a macro \c CASE(T) to be defined where \c T is a type parameter.
 *  Requires a macro \c FAIL to be defined that handles when an invalid \a type_id is used.
 *  \param[in] type_id Must be a valid nd_type_id_t.
 */
#define TYPECASE(type_id) \
switch(type_id) \
{            \
  case u8_id :CASE(uint8_t ); break; \
  case u16_id:CASE(uint16_t); break; \
  case u32_id:CASE(uint32_t); break; \
  case u64_id:CASE(uint64_t); break; \
  case i8_id :CASE(int8_t ); break; \
  case i16_id:CASE(int16_t); break; \
  case i32_id:CASE(int32_t); break; \
  case i64_id:CASE(int64_t); break; \
  case f32_id:CASE(float); break; \
  case f64_id:CASE(double); break; \
  default:   \
    FAIL("Unsupported pixel type.");    \
}
/** Requires a macro \c CASE2(T1,T2) to be defined where \c T1 and \c T2 are
 *  type parameters.
 *  Requires a macro \c FAIL to be defined that handles when an invalid \a type_id is used.
 *  \param[in] type_id Must be a valid nd_type_id_t.
 *  \param[in] T       A type name.  This should follow the u8,u16,u32,... form.  Usually
 *                     these types are defined in the implemenation function where this
 *                     macro is instanced.
 */
#define TYPECASE2(type_id,T) \
switch(type_id) \
{               \
  case u8_id :CASE2(T,uint8_t); break;  \
  case u16_id:CASE2(T,uint16_t); break; \
  case u32_id:CASE2(T,uint32_t); break; \
  case u64_id:CASE2(T,uint64_t); break; \
  case i8_id :CASE2(T,int8_t); break;  \
  case i16_id:CASE2(T,int16_t); break; \
  case i32_id:CASE2(T,int32_t); break; \
  case i64_id:CASE2(T,int64_t); break; \
  case f32_id:CASE2(T,float); break; \
  case f64_id:CASE2(T,double); break; \
  default:      \
    FAIL("Unsupported pixel type.");       \
}

int isaligned(unsigned x, unsigned n) { return (x%n)==0; }
int pipeline_exec(pipeline_t self, pipeline_image_t dst, const pipeline_image_t src, int *emit)
{ TRY(emit);

  TRY(isaligned(src->w,BX_));
  TRY(isaligned(src->h,BY_));
  TRY(isaligned(dst->w,BX_*WORK_));
  TRY(dst->h==2*src->h);

  { int count=0;
    CUTRY(cudaGetDeviceCount(&count));
    CUTRY(cudaSetDevice(count-1));
  }

  if(src->w>self->ctx.w)
  { TRY(pipeline_alloc_lut(self,src->w));
    TRY(pipeline_fill_lut(self,src->w));
  }
  pipeline_image_conversion_params(dst,src,self->invert,&self->m,&self->b);
  TRY(pipeline_upload(self,dst,src)); // updates context size and stride as well
// launch kernel
  #define CASE2(TSRC,TDST) launch<TSRC,TDST,BX_,BY_,WORK_>(self,emit)
  #define CASE(T)          TYPECASE2(src->type,T)
    { TYPECASE(dst->type); }
  #undef CASE
  #undef CASE2
  if(*emit)
    TRY(pipeline_download(self,dst));
  return 1;
Error:
  return 0;
}

#undef TYPECASE
#undef TYPECASE2
