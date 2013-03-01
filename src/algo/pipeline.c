#include "pipeline.h"
#include "pipeline-image.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if 0
#define ECHO(estr)   LOG("---\t%s\n",estr)
#else
#define ECHO(estr)
#endif
#define LOG(...)     printf(__VA_ARGS__)
#define REPORT(estr) LOG("%s(%d): %s()\n\t%s\n\tEvaluated to false.\n",__FILE__,__LINE__,__FUNCTION__,estr)
#define TRY(e)       do{ECHO(#e);if(!(e)){REPORT(#e);goto Error;}}while(0)
#define NEW(T,e,N)   TRY((e)=(T*)malloc(sizeof(T)*(N)))
#define ZERO(T,e,N)  memset((e),0,sizeof(T)*(N))
#define countof(e)   (sizeof(e)/sizeof(*(e)))

static unsigned bytes[]={1,2,4,8,1,2,4,8,4,8};

// Some constants to help with intensity scaling for type conversion and intensity inversion.
// Note: I don't actually care about scaling 64 bit types with the pipeline
#define POW2(bits) ((double)(1ULL<<bits))
static double range[]={
  POW2(8),POW2(16),POW2(32),(double)((unsigned long long)-1),
  POW2(8),POW2(16),POW2(32),(double)((unsigned long long)-1),
  1.0,1.0
};
static double mins[]={
  0.0,0.0,0.0,0.0,
  -POW2(7),-POW2(15),-POW2(31),-POW2(63),
  0.0,0.0
};
static double maxs[]={
  POW2(8)-1.0,POW2(16)-1.0,POW2(32)-1.0,(double)((unsigned long long)-1),
  POW2(7)-1.0,POW2(15)-1.0,POW2(31)-1.0,POW2(63)-1.0,
  1.0,1.0
};
#undef POW2


pipeline_image_t pipeline_make_empty_image()
{ pipeline_image_t out=0;
  NEW(struct pipeline_image_t_,out,1);
  ZERO(struct pipeline_image_t_,out,1);
  return out;
Error:
  return NULL;
}


/**
 * Maybe allocs the pipeline_image_t struct, does not allocate the buffer for intensity data.
 * @param  dst If this is not NULL, will reuse this struct.  Otherwise, allocates a new one.
 * @param  ctx The context parameters.
 * @param  src A description of the input image.
 * @return     dst on success, otherwise NULL
 */
pipeline_image_t pipeline_make_dst_image(pipeline_image_t dst, const pipeline_t ctx, const pipeline_image_t src)
{ unsigned w,h,nchan;
  TRY(ctx && src);
  if(!dst)
    TRY(dst=pipeline_make_empty_image());
  TRY(pipeline_get_output_dims(ctx,src,&w,&h,&nchan));
  dst->w=w;
  dst->h=h;
  dst->nchan=nchan;
  dst->type=src->type;
  dst->stride=w;
  dst->data=0;
  return dst;
Error:
  return NULL;
}


void pipeline_free_image(pipeline_image_t *self)
{ if(self && *self) {free(*self); *self=0;}
}

unsigned pipeline_image_nelem(pipeline_image_t self)
{ TRY(self);
  return self->h*self->w*self->nchan;
Error:
  return 0;
}

unsigned pipeline_image_nbytes(pipeline_image_t self)
{ return pipeline_image_nelem(self)*bytes[self->type];
}

void pipeline_image_conversion_params(pipeline_image_t dst,pipeline_image_t src,int invert,float *m,float *b)
{ const double xr = range[(long)src->type],
               yr = range[(long)dst->type],
               x0 = mins [(long)src->type],
               y0 = (invert?maxs:mins) [(long)dst->type],
               mm = (invert?-1.0:1.0)*yr/xr,
               bb = y0-mm*x0;
  if(m) *m=(float)mm;
  if(b) *b=(float)bb;
}

void pipeline_image_set_data  (pipeline_image_t self, void *data)
{ self->data=data;
}