/** \file
  Semi-private file describing an image for use with the pipeline.

  This should only be included in pipeline implementation files.
*/
#pragma once
#ifdef __cplusplus
extern "C" {
#endif

#ifdef _MSC_VER
#define __restrict__ __restrict
#endif

typedef enum pipeline_type_id_
{ u8_id,u16_id,u32_id,u64_id,
  i8_id,i16_id,i32_id,i64_id,
               f32_id,f64_id
} pipeline_type_id;

struct pipeline_image_t_
{ unsigned w,h,nchan,stride; // stride should be number of elements between rows (not bytes)
  pipeline_type_id type;
  void * __restrict__ data;
};

#ifdef __cplusplus
}
#endif