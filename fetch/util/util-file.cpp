#include "stdafx.h"

namespace w32file {

  i64 setpos( HANDLE hf, i64 pos, DWORD method )
  { LARGE_INTEGER li;
    li.QuadPart = pos;
    li.LowPart  = SetFilePointer( hf,
                                  li.LowPart,
                                 &li.HighPart,
                                  method );
    Guarded_Assert_WinErr( li.LowPart != INVALID_SET_FILE_POINTER &&
                           GetLastError() == NO_ERROR );
    return li.QuadPart;
  }

  i64 getpos( HANDLE hf )
  { return setpos(hf,0,FILE_BEGIN);
  }
}
