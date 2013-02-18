#pragma once
#ifdef __cplusplus
extern "C" {
#endif

#include "array.h"

pipeline_image_t pipeline_set_image_from_mylib_array(pipeline_image_t self,Array* src);

#ifdef __cplusplus
}
#endif