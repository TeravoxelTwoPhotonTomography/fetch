#pragma once

#ifdef __cplusplus
extern "C" {
#endif 

void m3Ncolminmaxf(const float *m, int ncols, float *mins, float *maxs);
void m4Ncolminmaxf(const float *m, int ncols, float *mins, float *maxs);
void m44mulf(const float a[16], const float *b, int ncols, float *out);
void m44mulf_affine(const float a[16], const float *b, int ncols, float *out);
int m44invf(const float m[16], float invOut[16]);
void m44f_copy(float *dst,const float *src);
void m44f_translate_left(float *src, float dx, float dy, float dz);
void m44f_translate_right(float *src, float dx, float dy, float dz);

#ifdef __cplusplus
}
#endif