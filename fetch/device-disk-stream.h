#include "stdafx.h"
//
// Can have multiple streams.  Need to manage the open devices.
//

typedef struct _disk_stream
  { char[1024]  path;
    char[4]     mode;
    HANDLE      hfile;
  } Disk_Stream;

typedef struct _disk_stream_index_item
  { char[32]    key;
    Disk_Stream item;
    Device      device;
  } Disk_Stream_Index_Item;

//
// Device interface
//

void         Disk_Stream_Init                (void);     // Only call once - alloc's a global
unsigned int Disk_Stream_Destroy             (void);     // Only call once -  free's a global
unsigned int Disk_Stream_Detach              (void);     // closes device context - waits for device to disarm
unsigned int Disk_Stream_Detach_Nonblocking  (void);     // closes device context - pushes close request to a threadpool
unsigned int Disk_Stream_Attach              (const char* filename, const char* mode);     // opens device context

//
// Tasks
//
DeviceTask* Disk_Stream_Create_Write_Task(void);

//
// Windows
//    testing utilities
//

#define IDM_DISK_STREAM               WM_APP+30
#define IDM_DISK_STREAM_DETACH        IDM_DISK_STREAM+1
#define IDM_DISK_STREAM_ATTACH        IDM_DISK_STREAM+2
#define IDM_DISK_STREAM_TASK_STOP     IDM_DISK_STREAM+4
#define IDM_DISK_STREAM_TASK_RUN      IDM_DISK_STREAM+5

#define IDM_DISK_STREAM_TASK_0        IDM_DISK_STREAM+10

void             Disk_Stream_UI_Append_Menu  ( HMENU hmenu );
void             Disk_Stream_UI_Insert_Menu  ( HMENU menu, UINT uPosition, UINT uFlags );
LRESULT CALLBACK Disk_Stream_UI_Handler ( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);