

#if 1
#define ECHO(estr)   LOG("---\t%s\n",estr)
#else
#define ECHO(estr)
#endif

#define LOG(...)     printf(__VA_ARGS__)
#define REPORT(estr) LOG("%s(%d): %s()\n\t%s\n\tEvaluated to false.\n",__FILE__,__LINE__,__FUNCTION__,estr)
#define TRY(e)       do{ECHO(#e);if(!(e)){REPORT(#e);goto Error;}}while(0)

#define CUREPORT(ecode,estr) LOG("%s(%d): %s()\n\t%s\n\t%s\n",__FILE__,__LINE__,__FUNCTION__,estr,cudaGetErrorString(ecode))
#define CUTRY(e)             do{ cudaError_t ecode; ECHO(#e); ecode=(e); (if(ecode!=cudaSuccess){CUREPORT(ecode,#e);goto Error;}}while(0)

#define NEW(T,e,N)   TRY((e)=(T*)malloc(sizeof(T)*(N)))
#define ZERO(T,e,N)  memset((e),0,sizeof(T)*(N))

#define countof(e)   (sizeof(e)/sizeof(*(e)))

// Original pipeline steps
// 1. pixel average (downsample)
// 2. frame average
// 3. cast to u16 (output pixel type)
// 4. invert
// 5. format frame (planewise...this will be implicit)
// 6. wrap
// 7. warp (lut)

/* PLAN
  INPUT
  * Count on 32MB/image (4 channels, no downsampling)
  * Expect 32 pixel aligned rows (16 after wrap)
      - one block will access BX*WORK columns

  1. VRAM based accumulator for frame averaging
     Just need the space for one extra image.
     Call to kernel increments accumulalator which will write an image
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

struct pipeline_t
{ float* accumulalator; ///< device memory, the size of one image.  Allocate with enough memory so rows are aligned to BX*WORK.
  float    norm;        ///< 1.0/the frame count as a float - set by launcher (eg. for ctx.every=4, this should be 0.25)
           m,b;         ///< slope and intercept for intensity scaling
  unsigned every,       ///< the number of frames to average
           count,       ///< the number of frames that have been pushed to the accumulator
           emit;        ///< true when (count+1)%every==0 - set by launcher
  unsigned turn;

  unsigned stride;      ///< number of elements between rows of input.  output stride will be stride/2.
  unsigned w,h;         ///< input width and height (height is nrows*nchan)
};

__constant__ unsigned *lut_;       ///< look up table for unwarp (ctx.w/2 number of elements).  Set by launch on first call or if width changes.
__constant__ float    *lut_norms_; ///< weight per pixel element

template<typename T,    ///< pixel type (input and output)
         unsigned BX,   ///< block size in X
         unsigned BY,   ///< block size in Y (channel dimension unrolled into Y)
         unsigned WORK  ///< number of elements to process per thread
         >
__global__ void __launch_bounds__(BX*BY,1) /* max threads, min blocks */
         pipeline_kernel(pipeline_t ctx, const T* __restrict__ src, T* __restrict__ dst)
{ __shared__ float buf[BY][WORK*BX+1];
  const int ox=threadIdx.x+blockIdx.x*WORK*BX,
            oy=threadIdx.y+blockIdx.y*BY;
  // --- Read ---
  if(oy>=h)
    return;

  src+=ox+oy*ctx.istride;
  if(blockIdx.x!=(gridDim.x-1))
  { 
    #pragma unroll
    for(int i=0;i<WORK;++i)
      buf[threadIdx.y][threadIdx.y+i*BX]=src[i*BX];
  } else // last block, need to check bounds
  { unsigned rem=(ctx.w-blockIdx.x*WORK*BX)/BX;  // divisible if data is aligned to BX
    for(int i=0;i<rem;++i)
      buf[threadIdx.y][threadIdx.x+i*BX]=src[i*BX];
  }
  __syncthreads();
  
  // --- Accumulate ---  
  if(ctx.every==1) // no frame averaging
  { 
    #pragma unroll
    for(int i=0;i<WORK;++i)                // put frame average in buffer
      buf[threadIdx.y][threadIdx.x+i*BX]=fma(buf[threadIdx.y][threadIdx.x+i*BX],m,b);
  } else {
    float *acc=ctx.acc+ox+oy*ctx.istride;
    #pragma unroll
    for(int i=0;i<WORK;++i)                // accumulate
      acc[i*BX]+=buf[threadIdx.y][threadIdx.x+i*BX];
    if(!ctx.emit)
      return;
    #pragma unroll
    for(int i=0;i<WORK;++i)                // put frame average in buffer
      buf[threadIdx.y][threadIdx.x+i*BX]=fma(acc[i*BX],m*ctx.norm,b);
  }

  // --- Wrap and Unwarp ---
  // *** FIXME - since this is an averaging step need this to go into a float buffer
  //             that gets mapped to dst later when it can be cast.
  // *** FIXME - fractional overlap of samples
  //           - can do this by tracking fractional part of lut and contributing
  //             to the lut[i] pixel and lut[i]+1'th pixel.
  //           - need to work out norms...norms has to be in dest space
  if(blockIdx.x < ctx.turn/(WORK*BX))      // --- block all forward scan
  { unsigned *lut   = lut_      +ox;
    float    *norms = lut_norms_+ox;
    dst+=oy*ctx.stride;                    // output to even lines (halved stride cancels with doubled line #)
    #pragma unroll
    for(int i=0;i<WORK;++i)
      dst[lut[i*BX]]+=buf[threadIdx.y][threadIdx.x+i*BX]*norms[i*BX];
  }
  else if(blockIdx.x > ctx.turn/(WORK*BX)) // --- block all reverse scan
  { unsigned *lut   = lut_      +ctx.turn-ox;
    float    *norms = lut_norms_+ctx.turn-ox;
    dst+=(2*oy+1)*ctx.stride/2;            // output to odd lines
    #pragma unroll
    for(int i=WORK-1;i>=0;--i)
      dst[lut[-i*BX]]+=buf[threadIdx.y][threadIdx.x+i*BX]*norms[-i*BX];    
  } else                                   // --- Block stradles turn
  {

  }
}

