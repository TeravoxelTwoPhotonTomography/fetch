
#include "types.h"
#include "MY_TIFF\tiff.image.h"

namespace mylib
{ 
  #include <array.h>
  // These manipulate a global critical section intended to be used
  // to synchronize mylib access.
  void lock(void);
  void unlock(void);
}//end namespace mylib

namespace mytiff
{
  int islsm(const char* fname);
  Basic_Type_ID pixel_type(Tiff_Image *tim);

  void lock(void);
  void unlock(void);
}
