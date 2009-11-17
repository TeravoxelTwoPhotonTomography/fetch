#include "stdafx.h"

//
// UTILITIES
//

typedef struct _tic_toc_timer
{ u64 last, // last observation on the counter
      rate; // counts per second  
} TicTocTimer;

TicTocTimer tic(void);
double      toc(TicTocTimer *last); // returns time since last in seconds and updates timer

//
// Windows
//    testing utilities
//


#define IDM_TESTS          WM_APP+128

void             Tester_Append_Menu  ( HMENU hmenu );
LRESULT CALLBACK Tester_Menu_Handler ( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);