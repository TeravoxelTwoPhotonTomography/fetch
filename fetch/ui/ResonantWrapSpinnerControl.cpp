/*
 * ResonantWrapSpinnerControl.cpp
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
#include "stdafx.h"
#include "ResonantWrapSpinnerControl.h"

#define _ID_RESONANTWRAP_SUBCONTROL_EDIT       1
#define _ID_RESONANTWRAP_SUBCONTROL_STATIC     2
#define _ID_RESONANTWRAP_SUBCONTROL_SPINNER    3
#define _ID_RESONANTWRAP_SUBCONTROL_BUTTON_OK  4
#define _ID_RESONANTWRAP_SUBCONTROL_BUTTON_OPT 5

namespace fetch
{

  namespace ui
  {

    ResonantWrapSpinnerControl::
    ResonantWrapSpinnerControl(worker::ResonantWrapAgent *target)
    : target(target)
    {
    }

    void
    ResonantWrapSpinnerControl::
    RegisterControl( HINSTANCE hInstance )
    { WNDCLASSEX wcex;
      wcex.cbSize           = sizeof( WNDCLASSEX );
      wcex.style            = CS_HREDRAW | CS_VREDRAW;
      wcex.lpfnWndProc      = &ResonantWrapSpinnerControl::WndProc;
      wcex.cbClsExtra       = 0;
      wcex.cbWndExtra       = 0;
      wcex.hInstance        = hInstance;
      wcex.hIcon            = NULL;
      wcex.hCursor          = LoadCursor( NULL, IDC_ARROW );
      wcex.hbrBackground    = ( HBRUSH )( COLOR_WINDOW + 1 );
      wcex.lpszMenuName     = NULL;
      wcex.lpszClassName    = "UIResonantWrapSpinnerControlClass";
      wcex.hIconSm          = NULL;
      Guarded_Assert_WinErr( RegisterClassEx( &wcex ) );
    }

    void
    ResonantWrapSpinnerControl::
    CreateControl( HWND parent, int top, int left, unsigned identifier )
    { RECT rc = {left, top, left+100, top+100},
          trc;
      HWND hwnd;
      HINSTANCE hinst = GetModuleHandle(NULL);
      HGDIOBJ  hfDefault;
      char lbl[] = "Resonant turn (0.1 px):";

      // get default font
      Guarded_Assert_WinErr( hfDefault = GetStockObject(DEFAULT_GUI_FONT));

      hwnd = CreateWindowEx( 0,                           // extended styles
                            "UIResonantWrapSpinnerControlClass",
                            "",                          // window name
                            WS_CHILD                     // styles
                            | WS_VISIBLE
                            | WS_BORDER,
                            rc.left, rc.top,             // x,y
                            rc.right - rc.left,          // w   (approx.  These get reset soon).
                            rc.bottom - rc.top,          // h
                            parent,
                            (HMENU) identifier,          // child window identifier
                            hinst,                       // hinstance
                            this );                      // lParam for WM_CREATE
      Guarded_Assert_WinErr( hwnd );
      this->self = hwnd;

      //
      // make the spinner control
      //    a static label next to an edit box/spinner and a button
      //    i.e.
      //       ,___ Radio button ind. oob     ,__ spinner ,____ button
      //      /                              /           /
      //    [ ] Resonant turn (0.1 px): [ 8770 ]^  [Optimize]
      //

      // Static control
      hwnd = NULL;
      hwnd = CreateWindowEx(0,
                            "Static",
                            "",
                            WS_CHILD
                            | WS_VISIBLE
                            | SS_RIGHT,
                            0,0,
                            10,10,             // width, height guess.  This will be corrected soon.
                            this->self,
                            (HMENU) _ID_RESONANTWRAP_SUBCONTROL_STATIC,
                            hinst,
                            NULL );
      Guarded_Assert_WinErr(hwnd);
      // Size static control and set text
      SetWindowFont( hwnd, hfDefault, FALSE );
      { HDC hdc = GetDC(hwnd);
        SIZE sz;
        GetTextExtentPoint32( hdc, lbl, sizeof(lbl), &sz );
        MoveWindow( hwnd, rc.left-left, rc.top-top,sz.cx,sz.cy,TRUE );
        ReleaseDC(hwnd,hdc);
      }
      SetWindowText( hwnd, lbl );
      GetClientRect( hwnd, &trc );
      rc.bottom = trc.bottom + rc.top;
      rc.right  = trc.right + rc.left;

      // edit box
      hwnd = NULL;
      Guarded_Assert_WinErr(
        hwnd = CreateWindowEx( 0,
                              "EDIT",
                              "",                       // window name
                              WS_CHILD
                              | WS_VISIBLE
                              | ES_CENTER
                              | WS_TABSTOP,
                              rc.right-left, 0,
                              50, 20,                     // pos and dims (approx.  These get reset next).
                              this->self,                 // parent
                              (HMENU) _ID_RESONANTWRAP_SUBCONTROL_EDIT, // child window identifier
                              hinst,
                              NULL ));
      // Size edit control
      SetWindowFont( hwnd, hfDefault, FALSE );
      { HDC hdc = GetDC(hwnd);
        SIZE sz;
        char str[] = "MMMMM";
        GetTextExtentPoint32( hdc, str, sizeof(str), &sz );
        MoveWindow( hwnd, trc.right-left, 0 ,(int)(sz.cx*1.1),(int)(sz.cy*1.1),TRUE );
        ReleaseDC(hwnd,hdc);
      }
      this->edit = hwnd;
      GetClientRect( hwnd, &trc );
      rc.bottom = MAX(rc.bottom-rc.top,trc.bottom) + rc.top;
      rc.right += trc.right + rc.left;

      // spinner
      hwnd = NULL;
      Guarded_Assert_WinErr(
        hwnd = CreateUpDownControl( WS_CHILD
                                    | WS_VISIBLE
                                    | WS_BORDER
                                    | UDS_ALIGNRIGHT
                                    | UDS_ARROWKEYS
                                    | UDS_HOTTRACK
                                    | UDS_SETBUDDYINT
                                    | UDS_NOTHOUSANDS,
                                    rc.right-left+10, 0,
                                    10, 0,
                                    this->self,          //parent
                                    _ID_RESONANTWRAP_SUBCONTROL_SPINNER, // child id
                                    hinst,
                                    this->edit,          //buddy
                                    10000,               // max
                                    0,                   // min
                                    0 ));                // initial value (off is safest)
      this->spin = hwnd;
      GetClientRect( hwnd, &trc );
      rc.bottom = MAX(rc.bottom-rc.top,trc.bottom)+rc.top;
      //rc.right += trc.right;
      
      // Button
      hwnd = NULL;
      { char text[] = "Ok";
        Guarded_Assert_WinErr(
          hwnd = CreateWindowEx( 0,
                                "BUTTON",
                                text,                     // button text
                                WS_CHILD
                                | WS_VISIBLE
                                | WS_TABSTOP
                                | BS_DEFPUSHBUTTON,       // responds to enter key
                                rc.right-left, 0,
                                50, 20,                   // pos and dims (approx.  These get reset next).
                                this->self,               // parent
                                (HMENU) _ID_RESONANTWRAP_SUBCONTROL_BUTTON_OK, // child window identifier
                                hinst,
                                NULL ));
        this->btn = hwnd;
        // Size button control
        SetWindowFont( hwnd, hfDefault, FALSE );
        { HDC hdc = GetDC(hwnd);
          SIZE sz;
          GetTextExtentPoint32( hdc, text, sizeof(text), &sz );
          MoveWindow( hwnd, rc.right-left, 0,(int)(sz.cx*1.1),(int)(sz.cy*1.1),TRUE );
          ReleaseDC(hwnd,hdc);
        }
        GetClientRect( hwnd, &trc );
        rc.bottom = MAX(rc.bottom-rc.top,trc.bottom) + rc.top;
        rc.right += trc.right + rc.left;
      }
      
      // Button
      hwnd = NULL;
      { char text[] = "Optimize";
        Guarded_Assert_WinErr(
          hwnd = CreateWindowEx( 0,
                                "BUTTON",
                                text,                     // button text
                                WS_CHILD
                                | WS_VISIBLE
                                | WS_TABSTOP
                                | BS_DEFPUSHBUTTON,       // responds to enter key
                                rc.right-left, 0,
                                50, 20,                   // pos and dims (approx.  These get reset next).
                                this->self,               // parent
                                (HMENU) _ID_RESONANTWRAP_SUBCONTROL_BUTTON_OPT, // child window identifier
                                hinst,
                                NULL ));
        this->btn = hwnd;
        // Size button control
        SetWindowFont( hwnd, hfDefault, FALSE );
        { HDC hdc = GetDC(hwnd);
          SIZE sz;
          GetTextExtentPoint32( hdc, text, sizeof(text), &sz );
          MoveWindow( hwnd, rc.right-left, 0,(int)(sz.cx*1.1),(int)(sz.cy*1.1),TRUE );
          ReleaseDC(hwnd,hdc);
        }
        GetClientRect( hwnd, &trc );
        rc.bottom = MAX(rc.bottom-rc.top,trc.bottom) + rc.top;
        rc.right += trc.right + rc.left;
      }
      
      Guarded_Assert_WinErr( MoveWindow( this->self, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, TRUE ));

      return;
    }

    void
    ResonantWrapSpinnerControl::
    OnUpdate(HWND hWnd)
    { BOOL  ok = FALSE;
      UINT val = GetDlgItemInt(hWnd,_ID_RESONANTWRAP_SUBCONTROL_EDIT,&ok,FALSE);
      
      if(ok)
      { float turn = val/10.0f;
        debug("ResonantWrapSpinnerControl - update turning point. Value: %f px\r\n", turn);
        this->target->Set_Turn_NonBlocking(turn);
      } else
      { Guarded_Assert_WinErr(0);
      }
    }

    void
    ResonantWrapSpinnerControl::
    OnOptimizeButtonClicked(HWND hWnd)
    {  // TODO
      debug("Optimize button clicked\r\n");
    }

    LRESULT CALLBACK
    ResonantWrapSpinnerControl::
    WndProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
    { int wmId, wmEvent;
      PAINTSTRUCT ps;
      HDC hdc;
      ResonantWrapSpinnerControl *self;

      // Move "self" pointer to window user data, or get the "self" pointer.
      if( message == WM_NCCREATE )
      { LPCREATESTRUCT cs = (LPCREATESTRUCT) lParam;
        SetWindowLong(hWnd, GWL_USERDATA, (LONG) cs->lpCreateParams);
      } else
      { Guarded_Assert_WinErr( self = (ResonantWrapSpinnerControl*) GetWindowLong(hWnd, GWL_USERDATA));
      }

      switch( message )
      {
      case WM_PAINT:
        hdc = BeginPaint( hWnd, &ps );
        EndPaint( hWnd, &ps );
        break;

      case WM_DESTROY:
        PostQuitMessage( 0 );
        break;

      case WM_SIZE:
        break;

      case WM_COMMAND:
        wmId    = LOWORD(wParam);
        wmEvent = HIWORD(wParam);
        // Parse the menu selections:
        switch (wmId)
        {
          case _ID_RESONANTWRAP_SUBCONTROL_BUTTON_OPT:
            switch(wmEvent)
            { case BN_CLICKED: self->OnOptimizeButtonClicked(hWnd); break;
            }
            break;

          case _ID_RESONANTWRAP_SUBCONTROL_BUTTON_OK:
            switch(wmEvent)
            { case BN_CLICKED: self->OnUpdate(hWnd); break;
            }
            break;

          default:
            return DefWindowProc(hWnd, message, wParam, lParam);
        }
        break;


      default:
          return DefWindowProc( hWnd, message, wParam, lParam );
        }

        return 0;
    }



  }

}
