/*
 * ResonantWrapSpinnerControl.h
 *
 *  Created on: May 17, 2010
 *      Author: Nathan Clack <clackn@janelia.hhmi.org>
 */
/*
 * Copyright 2010 Howard Hughes Medical Institute.
 * All rights reserved.
 * Use is subject to Janelia Farm Research Campus Software Copyright 1.1
 * license terms (http://license.janelia.org/license/jfrc_copyright_1_1.html).
 */

#pragma once

#include "../workers/ResonantWrap.h"

namespace fetch
{

  namespace ui
  {

    class ResonantWrapSpinnerControl
    {
      public:
        ResonantWrapSpinnerControl(worker::ResonantWrapAgent *target);

        static void RegisterControl( HINSTANCE hInstance );
        void CreateControl( HWND parent, int top, int left, unsigned identifier );
        static LRESULT CALLBACK WndProc( HWND, UINT, WPARAM, LPARAM ); // TODO: move this guy out of the interface and into the source.

        void OnUpdate(HWND hWnd);
        void OnOptimizeButtonClicked(HWND hWnd);

      public:
        worker::ResonantWrapAgent *target;
        HWND self,
             lbl,
             edit,
             spin,
             btn;
    };

  }

}

