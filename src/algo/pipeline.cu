#include "pipeline.h"
#include "pipeline-image.h"
#define _USE_MATH_DEFINES
#include <math.h>
#include <stdio.h>  //for printf
#include <stdlib.h> //for malloc
#include <string.h> //for memset

#include "cuda_runtime.h"

#define BX_   (32)
#define BY_   (32)
#define WORK_ (8)

#if 1
#define ECHO(estr)   LOG("---\t%s\n",estr)
#else
#define ECHO(estr)
#endif

#define LOG(...)     printf(__VA_ARGS__)
#define REPORT(estr,msg) LOG("%s(%d): %s()\n\t%s\n\t%s\n",__FILE__,__LINE__,__FUNCTION__,estr,msg)
#define TRY(e)       do{ECHO(#e);if(!(e)){REPORT(#e,"Evaluated to false.");goto Error;}}while(0)
#define FAIL(msg)    do{REPORT("Failure.",msg);goto Error;}} while(0)
#define NEW(T,e,N)   TRY((e)=(T*)malloc(sizeof(T)*(N)))
#define ZERO(T,e,N)  memset((e),0,sizeof(T)*(N))

#define CUREPORT(ecode,estr) LOG("%s(%d): %s()\n\t%s\n\t%s\n",__FILE__,__LINE__,__FUNCTION__,estr,cudaGetErrorString(ecode))
#define CUTRY(e)             do{ cudaError_t ecode; ECHO(#e); ecode=(e); if(ecode!=cudaSuccess){CUREPORT(ecode,#e);goto Error;}}while(0)
#define CUWARN(e)            do{ cudaError_t ecode; ECHO(#e); ecode=(e); if(ecode!=cudaSuccess){CUREPORT(ecode,#e);           }}while(0)
#define CUNEW(T,e,N)    CUTRY(cudaMalloc((void**)&(e),sizeof(T)*(N)))
#define CUZERO(T,e,N)   CUTRY(cudaMemset((e),0,sizeof(T)*(N)))


#define countof(e)   (sizeof(e)/sizeof(*(e)))


/**
 * The parameter collection that gets passed to the kernel
 */
struct pipeline_ctx_t
{ float    * __restrict__ accumulator; ///< device memory, the size of one source image.  Allocate with enough memory so rows are aligned to BX*WORK.
  unsigned * __restrict__ lut;         ///< look up table for unwarp (ctx.w/2 number of elements).  Set by launch on first call or if width changes.
  float    * __restrict__ lut_fpart;   ///< fractional part of pixel position for lut.  If a smaple is mapped to 4.6, then this is 0.6.  0.4 of the sample will go to pixel 4, and 0.6 will go to pixel 5
  float    * __restrict__ lut_norms;   ///< weight contributed by an input sample to it's main pixel and the neighbor.  Should be interleaved.  Should sum to 1 for each output pixel.
  float    norm,        ///< 1.0/the frame count as a float - set by launcher (eg. for ctx.every=4, this should be 0.25)
           m,b;         ///< slope and intercept for intensity scaling
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
  unsigned       alignment;      ///< output rows are aligned to this target number of elements.
  void  * __restrict__ src,  ///< device buffer
        * __restrict__ dst;  ///< device buffer
  float * __restrict__ tmp;  ///< device buffer
} *pipeline_t;

//
// --- DEVICE ---
//

template<typename T,unsigned BX,unsigned BY,unsigned WORK>
__device__ void load(const pipeline_ctx_t & ctx, const T* __restrict__ src, float * __restrict__ buf[BY],const unsigned ox, const unsigned oy)
{
  src+=ox+oy*ctx.istride;
  if(blockIdx.x!=(gridDim.x-1))
  { 
    #pragma unroll
    for(int i=0;i<WORK;++i)
      buf[threadIdx.y][threadIdx.y+i*BX]=src[i*BX];
  } else                                              // last block, need to check bounds
  { unsigned rem=(ctx.w-blockIdx.x*WORK*BX)/BX;       // divisible if data is aligned to BX
    for(int i=0;i<rem;++i)
      buf[threadIdx.y][threadIdx.x+i*BX]=src[i*BX];
    __syncthreads();
  }
}

template<typename T,unsigned BX,unsigned BY,unsigned WORK,bool EMIT_FRAME_AVERAGE>
__device__ void accumulate(const pipeline_ctx_t & ctx,float* __restrict__ buf[BY],const unsigned ox, const unsigned oy)
{
  float *acc=ctx.accumulator+ox+oy*ctx.istride;
  #pragma unroll
  for(int i=0;i<WORK;++i)                // accumulate
    acc[i*BX]+=buf[threadIdx.y][threadIdx.x+i*BX];
  if(!EMIT_FRAME_AVERAGE)
    return;
  #pragma unroll
  for(int i=0;i<WORK;++i)                // put frame average in buffer
    buf[threadIdx.y][threadIdx.x+i*BX]=fma(acc[i*BX],ctx.m*ctx.norm,ctx.b);
  #pragma unroll
  for(int i=0;i<WORK;++i)                // reset accumulator
    acc[i*BX]=0;
}

/**
 * dst width should be aligned to BX*WORK
 * lut takes care of wrapping and turn and whatnot
 * BY must be greater than or equal to 3.
 * 
 * TODO(?): should buffer the load from shared memory for the lut,fpart and norms, maybe?
 */
template<typename T,unsigned BX,unsigned BY,unsigned WORK>
__device__ void warp(const pipeline_ctx_t & ctx, const float * __restrict__ buf[BY], float *__restrict__ dst,const unsigned ox, const unsigned oy)
{ unsigned *lut   = ctx.lut      +  ox;
  float    *fpart = ctx.lut_fpart+  ox;
  float    *norms = ctx.lut_norms+2*ox;
  dst+=2*oy*ctx.ostride;
  #pragma unroll
  for(int i=0;i<WORK;++i)
  { dst[lut[i*BX]  ]+=buf[threadIdx.y][threadIdx.x+i*BX]*norms[2*i*BX  ]*fpart[i*BX];
    dst[lut[i*BX]+1]+=buf[threadIdx.y][threadIdx.x+i*BX]*norms[2*i*BX+1]*(1.0f-fpart[i*BX]);
  }
}

//
// --- KERNELS ---
//


/**
 * \param dst should be allocated with one extra (trash) column a the end.
 *            If w,h,c are the width, height, and number of channels of 
 *            src then d should have dimensions (w/2+1,2*h,c).
 */
template<typename T,    ///< pixel type (input and output)
         unsigned BX,   ///< block size in X
         unsigned BY,   ///< block size in Y (channel dimension unrolled into Y)
         unsigned WORK, ///< number of elements to process per thread
         bool     ENABLE_FRAME_AVERAGE,
         bool     EMIT_FRAME_AVERAGE     ///< true when (count+1)%every==0 - set by launcher
         >
__global__ void __launch_bounds__(BX*BY,1) /* max threads, min blocks */
pipeline_kernel(pipeline_ctx_t ctx, const T* __restrict__ src, float* __restrict__ dst)
{ __shared__ float buf[BY][WORK*BX+1]; // for pre-warp
  const unsigned ox=threadIdx.x+blockIdx.x*WORK*BX,
                 oy=threadIdx.y+blockIdx.y*BY;
  if(oy>=ctx.h)
    return;
  load<T,BX,BY,WORK>(ctx,src,buf,ox,oy);
  if(ENABLE_FRAME_AVERAGE)
  { accumulate<T,BX,BY,WORK,EMIT_FRAME_AVERAGE>(ctx,buf,ox,oy);
    if(!EMIT_FRAME_AVERAGE)
      return;
  } else 
  { 
    #pragma unroll
    for(int i=0;i<WORK;++i) // in-place intensity scaling
      buf[threadIdx.y][threadIdx.x+i*BX]=fma(buf[threadIdx.y][threadIdx.x+i*BX],ctx.m,ctx.b);
  }
  warp<T,BX,BY,WORK>(ctx,buf,dst,ox,oy);
}

/**
 * Cast array from float to T.
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
cast_kernel(T*__restrict__ dst, const float* __restrict__ src, unsigned stride)
{ __shared__ float buf[BY][WORK*BX+1];
  const int ox=threadIdx.x+blockIdx.x*WORK*BX,
            oy=threadIdx.y+blockIdx.y*BY;
  //if(oy>=h) return; // for unaligned y, uncomment and add an argument to the kernel call
  src+=ox+oy*stride;  
  dst+=ox+oy*stride;

  #pragma unroll
  for(int i=0;i<WORK;++i) //LOAD
    buf[threadIdx.y][threadIdx.y+i*BX]=src[i*BX];
  for(int i=0;i<WORK;++i) //STORE
    dst[i*BX]=(T)(buf[threadIdx.y][threadIdx.y+i*BX]);
}

//
// --- PUBLIC INTERFACE ---
//

pipeline_t pipeline_make(const pipeline_param_t *params)
{ pipeline_t self=NULL;
  TRY(params);
  NEW(pipeline_t_,self,1);
  ZERO(pipeline_t_,self,1);
  self->every            = params->frame_average_count;
  self->samples_per_scan = params->sample_rate_MHz*1.0e6/(double)params->scan_rate_Hz;
  self->invert           = params->invert_intensity;
  self->downsample       = params->pixel_average_count;
  self->alignment        = BX_*WORK_;
  self->ctx.norm         = 1.0f/(float)self->every;
  return self;
Error:
  return NULL;
}

void pipeline_free(pipeline_t *self)
{ if(self && *self)
  { void *ptrs[]={self[0]->ctx.accumulator,
                  self[0]->ctx.lut,
                  self[0]->ctx.lut_fpart,
                  self[0]->ctx.lut_norms,
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
{ const double d=inwidth/self->samples_per_scan; // 1 - duty
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

int pipeline_get_output_dims(pipeline_t self, const pipeline_image_t src,unsigned *w, unsigned *h, unsigned *nchan)
{ TRY(self && src);
  if(nchan) *nchan=src->nchan;
  if(h)     *h=src->h*2;
  if(w)     TRY(*w=pipeline_get_output_width(self,src->w));
  return 1;
Error:
  return 0;
}

static int pipeline_alloc_accumulator(pipeline_t self, const pipeline_image_t src)
{ if(self->ctx.accumulator)
    cudaFree(self->ctx.accumulator);
  const unsigned w     = self->alignment*((src->w+self->alignment-1)/self->alignment); // increase number of columns to align
  const unsigned nelem = w*src->h*src->nchan;
  CUNEW(float,self->ctx.accumulator,nelem);
  CUZERO(float,self->ctx.accumulator,nelem);
  return 1;
Error:
  self->ctx.accumulator=NULL;
  return 0;
}

static int pipeline_alloc_lut(pipeline_t self, unsigned inwidth)
{ if(self->ctx.lut)
  { CUTRY(cudaFree(self->ctx.lut));
    CUTRY(cudaFree(self->ctx.lut_fpart));
    CUTRY(cudaFree(self->ctx.lut_norms));
  }
  CUNEW(unsigned,self->ctx.lut,        inwidth);
  CUNEW(float   ,self->ctx.lut_fpart,  inwidth);
  CUNEW(float   ,self->ctx.lut_norms,2*inwidth);
  return 1;
Error:
  self->ctx.lut=NULL;
  self->ctx.lut_fpart=NULL;
  self->ctx.lut_norms=NULL;
  return 0;

}


static int pipeline_fill_lut(pipeline_t self, unsigned inwidth)
{ int isok=1;
  unsigned * __restrict__ lut=0;
  float    * __restrict__ fpart=0,
           * __restrict__ norms=0,
           * __restrict__ hits=0;
  // useful constants
  const double        d = inwidth/self->samples_per_scan; // 1 - duty
  const double    halfd = d/2.0;
  const unsigned     ow = pipeline_get_output_width(self,inwidth);
  const double        s = (1.0-d)/(double)ow;
  const double     cosd = cos(M_PI*d);
  // alloc temporary space
  NEW(unsigned ,lut  ,inwidth);
  NEW(float    ,fpart,inwidth);
  NEW(float    ,norms,2*inwidth);
  NEW(float    ,hits ,ow+1);
  ZERO(unsigned,lut  ,inwidth);
  ZERO(float   ,fpart,inwidth);
  ZERO(float   ,norms,2*inwidth);
  ZERO(float   ,hits ,ow+1);

  // compute lookup
  for(unsigned i=0;i<inwidth;++i)
  { double phase=halfd+s*i;
    double v=(1.0-cos(2*M_PI*phase)-(1-cosd) )*ow/2.0/cosd;
    unsigned j;
    j = lut[i]   =   floor(v);
    fpart[i]     = v-floor(v);
    hits[j  ]+=fpart[i];
    hits[j+1]+=(1.0f-fpart[i]);
  }
  // compute norms
  for(unsigned i=0;i<inwidth;++i)
  { const unsigned j=lut[i];
    norms[2*i  ]=1.0f/hits[j  ];
    norms[2*i+1]=1.0f/hits[j+1];
  }

  // upload
  CUTRY(cudaMemcpy(self->ctx.lut      ,lut  ,  inwidth*sizeof(unsigned),cudaMemcpyHostToDevice));
  CUTRY(cudaMemcpy(self->ctx.lut_fpart,fpart,  inwidth*sizeof(float)   ,cudaMemcpyHostToDevice));
  CUTRY(cudaMemcpy(self->ctx.lut_norms,norms,2*inwidth*sizeof(float)   ,cudaMemcpyHostToDevice));

Finalize:  
  if(lut)   free(lut);
  if(fpart) free(fpart);
  if(norms) free(norms);
  if(hits)  free(hits);
  return isok;
Error:
  isok=0;
  goto Finalize;
}

static int pipeline_upload(pipeline_t self, pipeline_image_t dst, const pipeline_image_t src)
{ if(self->src && (self->ctx.w>src->w || self->ctx.h>src->h))
  { CUTRY(cudaFree(self->src)); self->src=0;
    CUTRY(cudaFree(self->dst)); self->dst=0;
    CUTRY(cudaFree(self->tmp)); self->tmp=0;
  }
  if(!self->src)
  { CUTRY(cudaMalloc((void**)&self->src,pipeline_image_nbytes(src)));
    CUTRY(cudaMalloc((void**)&self->dst,pipeline_image_nbytes(dst)));
    CUTRY(cudaMalloc((void**)&self->tmp,pipeline_image_nelem(dst)*sizeof(float)));
  }
  CUTRY(cudaMemset(self->tmp,0,pipeline_image_nelem(dst)*sizeof(float)));
  CUTRY(cudaMemcpy(self->src,src->data,pipeline_image_nbytes(src),cudaMemcpyHostToDevice));
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
  CUTRY(cudaMemcpy(self->dst,dst->data,pipeline_image_nbytes(dst),cudaMemcpyDeviceToHost));
  return 1;
Error:
  return 0;
}


static template<typename Tsrc, typename Tdst,unsigned BX,unsigned BY,unsigned WORK>
int launch(pipeline_t self, int *emit)
{ unsigned ow=pipeline_get_output_width(self,self->ctx.w);
#define CEIL(num,den) (((num)+(den)-(1))/(den))  
  dim3 threads(BX,BY),
       blocks_up(CEIL(self->ctx.w,BX*WORK),CEIL(self->ctx.h,BY)), // for the main kernel
       blocks_down(CEIL(ow,BX*WORK),CEIL(2*self->ctx.h,BY));      // for the cast from tmp to dst
#undef CEIL       
  TRY(emit);
  if(self->count>1) // frame averaging enabled
  { if( ((self->count+1)%self->every)==0 )
    { *emit=1;
      pipeline_kernel<TSrc,BX,BY,WORK,1,1><<<blocks_up,threads>>>(self->ctx,self->src,self->tmp);
      cast_kernel<Tdst,BX,BY,WORK><<<blocks_down,threads>>>(self->dst,self->tmp);  
    } else
    { *emit=0;
      pipeline_kernel<TSrc,BX,BY,WORK,1,0><<<blocks_up,threads>>>(self->ctx,self->src,self->tmp);
    }
    self->count++;
  } else            // frame averaging disabled
  { if(emit) *emit=1;
    pipeline_kernel<TSrc,BX,BY,WORK,0,1><<<blocks_up,threads>>>(self->ctx,self->src,self->tmp);
    cast_kernel<Tdst,BX,BY,WORK><<<blocks_down,threads>>>(self->dst,self->tmp);
    CUTRY(cudaGetLastError());
  }
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
  case u8_id :CASE(uint8_t ); \
  case u16_id:CASE(uint16_t); \
  case u32_id:CASE(uint32_t); \
  case u64_id:CASE(uint64_t); \
  case i8_id :CASE(int8_t ); \
  case i16_id:CASE(int16_t); \
  case i32_id:CASE(int32_t); \
  case i64_id:CASE(int64_t); \
  case f32_id:CASE(float); \
  case f64_id:CASE(double); \
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
  case u8_id :CASE2(T,uint8_t);  \
  case u16_id:CASE2(T,uint16_t); \
  case u32_id:CASE2(T,uint32_t); \
  case u64_id:CASE2(T,uint64_t); \
  case i8_id :CASE2(T,int8_t);  \
  case i16_id:CASE2(T,int16_t); \
  case i32_id:CASE2(T,int32_t); \
  case i64_id:CASE2(T,int64_t); \
  case f32_id:CASE2(T,float); \
  case f64_id:CASE2(T,double); \
  default:      \
    FAIL"Unsupported pixel type.";       \
}

int isaligned(unsigned x, unsigned n) { return (x%n)==0; }
int pipeline_exec(pipeline_t self, pipeline_image_t dst, const pipeline_image_t src, int *emit)
{ TRY(emit);

  TRY(isaligned(src->w,BX));
  TRY(isaligned(src->h,BY));
  TRY(isaligned(dst->w,BX*WORK));
  TRY(dst->h==2*src->h);
  
  if(src->w>self->ctx.w)
  { TRY(pipeline_alloc_accumulator(self,src)); // these will free if one already exists
    TRY(pipeline_alloc_lut(self,src->w));
    TRY(pipeline_fill_lut(self,src->w));
  }
  pipeline_image_conversion_params(dst,src,self->invert,&self->ctx.m,&self->ctx.b);
  TRY(pipeline_upload(self,dst,src)); // updates context size and stride as well
// launch kernel
  #define CASE2(TSRC,TDST) TRY(launch<TSRC,TDST,BX,BY,WORK>(self,emit))
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

/* PLAN
  INPUT
  * Count on 32MB/image (4 channels, no downsampling)
  * Expect 32 pixel aligned rows (16 after wrap)
      - one block will access BX*WORK columns

  1. VRAM based accumulator for frame averaging
     Just need the space for one extra image.
     Call to kernel increments accumulator which will write an image
     to the destination buffer every k calls.

     * Accumulator is aligned to BX*WORK
  2. cast and invert are pixel-wise and can be done inline.  frame format is implicit
  3. wrap and unwarp are combined
    1. wrap
       Rows have an even number of pixels so turn will be aligned to a pixel border.  This
       makes this just a copy...might be able to do this in place.
    2. unwrap 
       This is a lut mapping...could introduce linear interp, though this could 
       be taken care of with the lut and the norm
    3. subsampling is done by lut

  THREADS PER BLOCK (BX,BY)
    - BX   should be 1 warp (32)
    - BY   should be set to run max threads per block (BY=1024/BX)
    - WORK should be just enough to hide memory latency
  BLOCKS
    X: ceil( width/BX/WORK )
    Y: ceil( height*nchan/BY)
*/