#pragma once
#ifdef __cplusplus
extern "C" {
#endif

typedef struct pipeline_t_       *pipeline_t;
typedef struct pipeline_image_t_ *pipeline_image_t;

typedef struct pipeline_param_t_
{ unsigned frame_average_count;
  unsigned pixel_average_count;
  unsigned invert_intensity;
  unsigned scan_rate_Hz,
           sample_rate_MHz;
} pipeline_param_t;

pipeline_t pipeline_make           (const pipeline_param_t *params);
void       pipeline_free           (pipeline_t *ctx);
int        pipeline_get_output_dims(pipeline_t ctx,
                                    const pipeline_image_t src,
                                    unsigned *w, unsigned *h, unsigned *nchan);
int        pipeline_exec           (pipeline_t ctx, pipeline_image_t dst, const pipeline_image_t src, int *emit);

pipeline_image_t pipeline_make_empty_image();
pipeline_image_t pipeline_make_dst_image(pipeline_image_t dst, const pipeline_t ctx, const pipeline_image_t src); // maybe allocs the pipeline_image_t struct, does not allocate the buffer for intensity data.
void             pipeline_free_image      (pipeline_image_t *self);
unsigned         pipeline_image_nelem     (pipeline_image_t  self);
unsigned         pipeline_image_nbytes    (pipeline_image_t  self);
void             pipeline_image_conversion_params(pipeline_image_t dst,pipeline_image_t src,int invert,float *m,float *b);
void             pipeline_image_set_data  (pipeline_image_t self, void *data);
#ifdef __cplusplus
}
#endif