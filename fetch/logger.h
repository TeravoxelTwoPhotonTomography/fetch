#pragma once

#include "resource.h"

#define LOGGER_CLASS_NAME _T("LOGGER")

ATOM Logger_RegisterClass              ( HINSTANCE hInstance );
HWND Logger_InitInstance               ( HINSTANCE hInstance, int nCmdShow);
void Logger_Push_Text                  ( const TCHAR* msg );
void Logger_Register_Default_Reporting ( void);