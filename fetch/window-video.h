#pragma once
#include "resource.h"

#define VIDEO_CLASS_NAME "VIDEO"

ATOM Video_RegisterClass              ( HINSTANCE hInstance );
HWND Video_InitInstance               ( HINSTANCE hInstance, int nCmdShow);