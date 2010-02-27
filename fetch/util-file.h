#pragma once
#include "stdafx.h"

namespace w32file {
  i64 getpos( HANDLE hf );
  i64 setpos( HANDLE hf, i64 pos, DWORD method );
}
