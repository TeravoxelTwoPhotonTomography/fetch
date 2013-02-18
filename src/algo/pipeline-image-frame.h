#include "frame.h"

typedef struct pipeline_image_t_ *pipeline_image_t;

pipeline_image_t
  pipeline_set_image_from_frame(pipeline_image_t self, fetch::Frame_With_Interleaved_Planes* f);


fetch::Frame_With_Interleaved_Planes*
  pipeline_format_frame(pipeline_image_t self, fetch::Frame_With_Interleaved_Planes* f);