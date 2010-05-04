/*
 * Author: Nathan Clack <clackn@janelia.hhmi.org>
 *   Date: Apr 20, 2010
 */
/*
 * Copyright 2010 Howard Hughes Medical Institute.
 * All rights reserved.
 * Use is subject to Janelia Farm Research Campus Software Copyright 1.1
 * license terms (http://license.janelia.org/license/jfrc_copyright_1_1.html).
 */
#pragma once
#include "../stdafx.h"
#include "../devices/pockels.h"

namespace fetch
{
  namespace ui
  {
    namespace pockels
    { 
  
    //struct UIControl
    //  { HWND self,
    //          lbl,
    //        edit,
    //        spin,
    //          btn;
    //  };

      
      class PockelsIntensitySpinControl
      { public:
          PockelsIntensitySpinControl(device::Pockels *pockels);
          
          static void      Spinner_RegisterClass( HINSTANCE hInstance );
          void             Spinner_CreateControl( HWND parent, int top, int left, unsigned identifier );
          static LRESULT CALLBACK Spinner_WndProc      ( HWND, UINT, WPARAM, LPARAM ); // TODO: move this guy out of the interface and into the source.
          
          void OnButtonClicked(HWND hWnd);
          
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
