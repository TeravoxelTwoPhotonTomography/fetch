#pragma once
#include "config.h"
#ifdef HAVE_ALAZAR
#ifdef __cplusplus
extern "C" {
#endif
#include <stdio.h>
typedef struct _alazar_t*     alazar_t;
typedef struct _alazar_cfg_t* alazar_cfg_t;
typedef struct _alazar_img_t* alazar_img_t;

alazar_cfg_t alazar_make_config();
void         alazar_free_config(alazar_cfg_t *cfg);
void         alazar_set_image_size(alazar_cfg_t cfg, unsigned scans_per_second,unsigned scans,double *duty);
void         alazar_set_channel_enable(alazar_cfg_t cfg, int iboard, int ichan, int isenabled);
void         alazar_set_channel_input_range(alazar_cfg_t cfg, int iboard, int ichan, unsigned rangeid);
void         alazar_set_line_trigger_lvl_volts(alazar_cfg_t cfg, double volts);
void         alazar_set_aux_out_board(alazar_cfg_t cfg, unsigned boardid);

void         alazar_get_image_size(alazar_t ctx, unsigned *w, unsigned *h);
size_t       alazar_get_image_size_bytes(alazar_t ctx);
void         alazar_set_line_trigger_lvl_volts(alazar_cfg_t cfg, double volts);

int alazar_attach(alazar_t *ctx);
int alazar_detach(alazar_t *ctx);
int alazar_arm   (alazar_t ctx, alazar_cfg_t cfg);
int alazar_disarm(alazar_t ctx);
int alazar_start (alazar_t ctx);
int alazar_stop  (alazar_t ctx);
int alazar_fetch (alazar_t ctx, void **buf, unsigned timeout_ms);

int  alazar_alloc_buf(alazar_t ctx, void **buf); // ctx must be armed.
void alazar_free_buf (void **buf);

int alazar_print_version(FILE* fp);

#ifdef __cplusplus
} //extern "C"
#endif

#else  // DONT HAVE_ALAZAR
typedef int alazar_t;
#endif //HAVE_ALAZAR