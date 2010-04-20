#pragma once
#include "stdafx.h"

#ifndef CALLBACK
#define CALLBACK
#endif

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
      
      class PockelsIntensitySpinControl {
      public:
        PockelsIntensitySpinControl(device::Pockels *pockels);

        static void      RegisterClass( HINSTANCE hInstance );
        void             CreateControl( HWND parent, int top, int left, unsigned identifier );
        LRESULT CALLBACK WndProc      ( HWND, UINT, WPARAM, LPARAM );

        void             OnButtonClicked(HWND hWnd);

      public:
        device::Pockels *pockels;
        HWND self,
              lbl,
             edit,
             spin,
              btn;
      };

    }
  }
}
