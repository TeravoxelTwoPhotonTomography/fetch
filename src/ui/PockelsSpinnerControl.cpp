#include "stdafx.h"
#include "PockelsSpinnerControl.h"
#include "../devices/scanner2D.h"

#define _ID_POCKELS_SUBCONTROL_EDIT    1
#define _ID_POCKELS_SUBCONTROL_STATIC  2
#define _ID_POCKELS_SUBCONTROL_SPINNER 3
#define _ID_POCKELS_SUBCONTROL_BUTTON  4

namespace fetch
{
  namespace ui
  {
    namespace pockels
    {
    
      PockelsIntensitySpinControl::
      PockelsIntensitySpinControl(device::Pockels *pockels)
        : pockels(pockels)
      {}

      void
      PockelsIntensitySpinControl::
      Spinner_RegisterClass( HINSTANCE hInstance )
      { WNDCLASSEX wcex;
        wcex.cbSize           = sizeof( WNDCLASSEX );
        wcex.style            = CS_HREDRAW | CS_VREDRAW;
        wcex.lpfnWndProc      = &Spinner_WndProc;
        wcex.cbClsExtra       = 0;
        wcex.cbWndExtra       = 0;
        wcex.hInstance        = hInstance;
        wcex.hIcon            = NULL;
        wcex.hCursor          = LoadCursor( NULL, IDC_ARROW );
        wcex.hbrBackground    = ( HBRUSH )( COLOR_WINDOW + 1 );
        wcex.lpszMenuName     = NULL;
        wcex.lpszClassName    = "UIPockelsControlClass";
        wcex.hIconSm          = NULL;
        Guarded_Assert_WinErr( RegisterClassEx( &wcex ) );
      }      
      
      void
      PockelsIntensitySpinControl::
      Spinner_CreateControl( HWND parent, int top, int left, unsigned identifier )
      { RECT rc = {left, top, left+100, top+100}, 
            trc;
        HWND hwnd;
        HINSTANCE hinst = GetModuleHandle(NULL);
        HGDIOBJ  hfDefault;
        char lbl[] = "Pockels (mV):";
        
        // get default font
        Guarded_Assert_WinErr( hfDefault = GetStockObject(DEFAULT_GUI_FONT));
        
        hwnd = CreateWindowEx( 0,                           // extended styles
                              "UIPockelsControlClass", 
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
        // make the pockels control
        // - child controls are positioned reletive to hwnd
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
                              (HMENU) _ID_POCKELS_SUBCONTROL_STATIC,
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
                                50, 20,                   // pos and dims (approx.  These get reset next).
                                this->self,               // parent
                                (HMENU) _ID_POCKELS_SUBCONTROL_EDIT, // child window identifier
                                hinst,
                                NULL ));
        // Size edit control
        SetWindowFont( hwnd, hfDefault, FALSE );
        { HDC hdc = GetDC(hwnd);
          SIZE sz;
          GetTextExtentPoint32( hdc, "MMMM", 4, &sz );
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
                                      | UDS_SETBUDDYINT,
                                      rc.right-left+10, 0,
                                      10, 0,
                                      this->self,            //parent
                                      _ID_POCKELS_SUBCONTROL_SPINNER, // child id
                                      hinst,
                                      this->edit,            //buddy
                                      2000,                // max
                                      0,                   // min
                                      0 ));                // initial value (off is safest)
        this->spin = hwnd;
        GetClientRect( hwnd, &trc );
        rc.bottom = MAX(rc.bottom-rc.top,trc.bottom)+rc.top;
        //rc.right += trc.right;

        // Button
        hwnd = NULL;
        Guarded_Assert_WinErr(
          hwnd = CreateWindowEx( 0,
                                "BUTTON",
                                "Apply",                  // button text
                                WS_CHILD 
                                | WS_VISIBLE
                                | WS_TABSTOP
                                | BS_DEFPUSHBUTTON,       // responds to enter key
                                rc.right, top,
                                50, 20,                   // pos and dims (approx.  These get reset next).
                                this->self,                 // parent
                                (HMENU) _ID_POCKELS_SUBCONTROL_BUTTON, // child window identifier
                                hinst,
                                NULL ));
        this->btn = hwnd;
        // Size button control
        SetWindowFont( hwnd, hfDefault, FALSE );
        { HDC hdc = GetDC(hwnd);
          SIZE sz;
          GetTextExtentPoint32( hdc, "Apply", 5, &sz );
          MoveWindow( hwnd, rc.right-left, 0 ,(int)(sz.cx*1.1),(int)(sz.cy*1.1),TRUE );
          ReleaseDC(hwnd,hdc);
        }
        GetClientRect( hwnd, &trc );
        rc.bottom = MAX(rc.bottom-rc.top,trc.bottom) + rc.top;
        rc.right += trc.right + rc.left;
        
        Guarded_Assert_WinErr( MoveWindow( this->self, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, TRUE ));

        return;
      }
      
      void
      PockelsIntensitySpinControl::
      OnButtonClicked(HWND hWnd)
      { BOOL  ok = FALSE;
        UINT val = GetDlgItemInt(hWnd,_ID_POCKELS_SUBCONTROL_EDIT,&ok,FALSE);                
        if(ok)
        { f64 volts = val/1000.0; 
          debug("Pockels edit change. Value: %f V\r\n", volts);
          if(this->pockels->Is_Volts_In_Bounds(volts))
            this->pockels->Set_Open_Val_Nonblocking(val/1000.0);
          else
          { warning("Value set for Pockels cell is out of bounds.\r\n");
            SetDlgItemInt(hWnd,
                          _ID_POCKELS_SUBCONTROL_EDIT,
                          (UINT) (this->pockels->config->v_open()*1000.0), //convert to mV
                          FALSE );                                      // unsigned
          }
        }
      }

      LRESULT CALLBACK
      PockelsIntensitySpinControl::
      Spinner_WndProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
      { int wmId, wmEvent;
        PAINTSTRUCT ps;
        HDC hdc;
        PockelsIntensitySpinControl *self;

        // Move "self" pointer to window user data, or get the "self" pointer.
        if( message == WM_NCCREATE )
        { LPCREATESTRUCT cs = (LPCREATESTRUCT) lParam; 
          SetWindowLong(hWnd, GWL_USERDATA, (LONG) cs->lpCreateParams);
        } else
        { Guarded_Assert_WinErr( self = (PockelsIntensitySpinControl*) GetWindowLong(hWnd, GWL_USERDATA));         
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
          case _ID_POCKELS_SUBCONTROL_BUTTON:
            switch(wmEvent)
            { case BN_CLICKED: self->OnButtonClicked(hWnd); break;
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
}


