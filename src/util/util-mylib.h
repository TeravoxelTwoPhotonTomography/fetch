namespace mylib
{ 
  #include <array.h>

  // These manipulate a global critical section intended to be used
  // to syncronize mylib access.
  void lock(void);
  void unlock(void);

}//end namespace mylib
