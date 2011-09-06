
#include "common.h"
#include "config.h"

namespace w32file {

  // Returns set file position normally.
  // Returns -1 if seek is past end of file
  // Panics otherwise.
  //
  // Should be ok with very large files
  i64 setpos( HANDLE hf, i64 pos, DWORD method )
  { LARGE_INTEGER in,out;
    //BOOL sts;
    in.QuadPart = pos;
    Guarded_Assert_WinErr( 
      SetFilePointerEx( hf,
                        in,
                       &out,
                        method));
    Guarded_Assert_WinErr(GetFileSizeEx(hf,&in));
    if(out.QuadPart>=in.QuadPart) //past end of file
      return -1;
    
    return out.QuadPart;
  }

  // Will return -1 if current position is past end of file.
  // Should work with files larger than 2GB
  i64 getpos( HANDLE hf )
  { return setpos(hf,0,FILE_CURRENT);
  }

  int eof( HANDLE hf )
  { return getpos(hf)<0;
  }
}
