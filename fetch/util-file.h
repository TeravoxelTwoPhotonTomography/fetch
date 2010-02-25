#pragma once
#include "stdafx.h"

namespace w32file {
  int64 getpos( HANDLE hf );
  int64 setpos( HANDLE hf, int64 pos, DWORD method );
}
