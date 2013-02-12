#include "pipeline.h"
#include "pipeline-image.h"
#include "frame.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if 1
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

const pipeline_type_id typetable[]=
{ u8_id,u16_id,u32_id,u64_id,
  i8_id,i16_id,i32_id,i64_id,
               f32_id,f64_id};

pipeline_image_t pipeline_set_image_from_frame(pipeline_image_t self, fetch::Frame_With_Interleaved_Planes* f)
{ TRY(self && f);
  self->w=f->width;
  self->h=f->height;
  self->nchan=f->nchan;
  self->stride=f->width*f->Bpp;
  self->type=typetable[(long)f->rtti];
  self->data=f->data;
  return self;
Error:
  return NULL;
}
