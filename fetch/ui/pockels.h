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
#include "stdafx.h"

namespace fetch
{
  namespace ui
  {
    namespace pockels
    { struct UIControl
      { HWND self,
              lbl,
            edit,
            spin,
              btn;
      };
      
                  void RegisterClass( HINSTANCE hInstance );
            UIControl CreateControl( HWND parent, int top, int left, unsigned identifier );
      LRESULT CALLBACK WndProc      ( HWND, UINT, WPARAM, LPARAM );
    }
  }
}
