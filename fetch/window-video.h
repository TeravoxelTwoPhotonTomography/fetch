#pragma once
#include "resource.h"
#include "stdafx.h"
#include "device.h"

#define VIDEO_CLASS_NAME "VIDEO"

HWND                Video_Display_Attach( HINSTANCE hInstance, int nCmdShow);
unsigned int        Video_Display_Release();
LRESULT CALLBACK    Video_Display_WndProc( HWND, UINT, WPARAM, LPARAM );
void                Video_Display_Render_One_Frame();

// Default event handlers
void                Video_Display_GUI_On_ID_COMMAND_VIDEODISPLAY(void);

// Data
void                Video_Display_Connect_Device( Device *source, int channel );