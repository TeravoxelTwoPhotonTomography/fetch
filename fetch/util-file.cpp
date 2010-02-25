#include "stdafx.h"

namespace w32file {

  int64 setpos( HANDLE hf, int64 pos, DWORD method )
  { LARGE_INTEGER li;
    li.QuadPart = distance;
    li.LowPart  = SetFilePointer( hf,
                                  li.LowPart,
                                 &li.HighPart,
                                  method );
    Guarded_Assert_WinErr( li.LowPart != INVALID_SET_FILE_POINTER &&
                           GetLastError() == NO_ERROR );
    return li.QuadPart;
  }

  int64 getpos( HANDLE hf )
  { return set_pos(hf,0,FILE_BEGIN);
  }
}
