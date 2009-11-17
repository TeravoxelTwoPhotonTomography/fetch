#include "stdafx.h"

//
// Windows
//    testing utilities
//


#define IDM_TESTS          WM_APP+128

void             Tester_Append_Menu  ( HMENU hmenu );
LRESULT CALLBACK Tester_Menu_Handler ( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);