#include "array.h"
#include "pipeline.h"
#include "pipeline-image.h"

#if 0
#define ECHO(estr)   LOG("---\t%s\n",estr)
#else
#define ECHO(estr)
#endif
#define LOG(...)     printf(__VA_ARGS__)
#define REPORT(estr) LOG("%s(%d): %s()\n\t%s\n\tEvaluated to false.\n",__FILE__,__LINE__,__FUNCTION__,estr)
#define TRY(e)       do{ECHO(#e);if(!(e)){REPORT(#e);goto Error;}}while(0)
   
static const pipeline_type_id typetable[]=
{ u8_id,u16_id,u32_id,u64_id,
  i8_id,i16_id,i32_id,i64_id,
               f32_id,f64_id};

pipeline_image_t pipeline_set_image_from_mylib_array(pipeline_image_t self, Array* a)
{ TRY(a && a->ndims>=2);
  if(!self)
    TRY(self=pipeline_make_empty_image());
  self->w      = a->dims[0];
  self->h      = a->dims[1];
  self->nchan  = (a->ndims>=3)?a->dims[2]:1;
  self->stride = a->dims[0];
  self->type   = typetable[(long)a->type];
  self->data   = a->data;
  return self;
Error:
  return NULL;
}