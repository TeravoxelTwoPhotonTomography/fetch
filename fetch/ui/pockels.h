#pragma once
#include "stdafx.h"

namespace fetch
{
  namespace ui
  {
    namespace pockels
    { typedef struct _control
      { HWND self,
              lbl,
            edit,
            spin,
              btn;
      } UIControl;
      
                  void RegisterClass( HINSTANCE hInstance );
            UIControl CreateControl( HWND parent, int top, int left, unsigned identifier );
      LRESULT CALLBACK WndProc      ( HWND, UINT, WPARAM, LPARAM );
    }
  }
}
