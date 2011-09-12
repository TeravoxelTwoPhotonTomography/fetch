#pragma once

#ifdef __cplusplus
namespace  mylib {
#endif

#include<array.h>

#ifdef __cplusplus
}
using namespace mylib;
#endif

#ifdef __cplusplus
extern "C" {
#endif


  /*
   * unwarp_get_dims_ip
   * <dims> [In/Out]
   *    Should initially be a copy of the input dimensions.
   *    After the function returns <dims> has the output dimensions.
   *    Only the first dimension is changed.
   */

void unwarp_get_dims(Dimn_Type *dims, Array *in, float duty);
void unwarp_get_dims_ip(Dimn_Type *dims, float duty);
int unwarp_gpu(Array *out, Array* in, float duty);
int unwarp_cpu(Array* out, Array* in, float duty);

#ifdef __cplusplus
}
#endif
