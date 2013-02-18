#include "pipeline.h"
#include "pipeline-image.h"
#include "frame.h"

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

const pipeline_type_id typetable[]=
{ u8_id,u16_id,u32_id,u64_id,
  i8_id,i16_id,i32_id,i64_id,
               f32_id,f64_id};
const Basic_Type_ID frametype[]= /* Gross -_- */
{ id_u8,
  id_u16,
  id_u32,
  id_u64,
  id_i8,
  id_i16,
  id_i32,
  id_i64,
  id_f32,
  id_f64
};

pipeline_image_t pipeline_set_image_from_frame(pipeline_image_t self, fetch::Frame_With_Interleaved_Planes* f)
{ TRY(f);
  if(!self)
    TRY(self=pipeline_make_empty_image());
  self->w=f->width;
  self->h=f->height;
  self->nchan=f->nchan;
  self->stride=f->width; // number of elements
  self->type=typetable[(long)f->rtti];
  self->data=f->data;
  return self;
Error:
  return NULL;
}

/**
 * Set the dimensions of f according to self.  May reallocate f.
 * @param  self          The reference image.
 * @param  f             The frame buffer to format.
 * @return               The possibly reallocated frame pointer on success, otherwise 0.
 */
fetch::Frame_With_Interleaved_Planes* pipeline_format_frame(pipeline_image_t self, fetch::Frame_With_Interleaved_Planes* f)
{ size_t oldbytes=f->size_bytes();
  fetch::Frame_With_Interleaved_Planes ref(self->w,self->h,self->nchan,frametype[self->type]);
  ref.format(f);
  if(oldbytes<f->size_bytes())
  { TRY(f=(fetch::Frame_With_Interleaved_Planes*)realloc(f,f->size_bytes()));
    ref.format(f);
  }
  return f;
Error:
  return NULL;
}