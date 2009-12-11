#pragma once
#include "resource.h"
#include "stdafx.h"

#define VIDEO_CLASS_NAME "VIDEO"

HWND                Video_Window_Attach( HINSTANCE hInstance, int nCmdShow);
HRESULT             Video_Window_Release();
LRESULT CALLBACK    Video_WndProc( HWND, UINT, WPARAM, LPARAM );
void                Video_Window_Render_One_Frame();