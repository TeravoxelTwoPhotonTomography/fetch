#pragma once
#include "stdafx.h"
#include "device.h"

//
// Can have multiple streams.  Need to manage the open devices.
//

typedef struct _disk_stream
  { char        path[1024];
    char        mode;
    HANDLE      hfile;
  } Disk_Stream;

#define DISK_STREAM_EMPTY {"\0","\0",INVALID_HANDLE_VALUE}

#define DISK_STREAM_ALIAS_LENGTH 32
typedef struct _disk_stream_index_item
  { char         alias[DISK_STREAM_ALIAS_LENGTH];
    Disk_Stream  item;
    Device      *device;
  } Disk_Stream_Index_Item;

#define DISK_STREAM_INDEX_ITEM_EMPTY = {"\0",DISK_STREAM_EMPTY,NULL};

TYPE_VECTOR_DECLARE(Disk_Stream_Index_Item);

//
// Device interface
//

void         Disk_Stream_Init                    (void);     // Only call once - alloc's global resources for module
unsigned int Disk_Stream_Destroy                 (void);     // Only call once -  free's global resources for module
unsigned int Disk_Stream_Detach                  (const char* alias);     // closes device context - waits for device to disarm
unsigned int Disk_Stream_Detach_All              (void);                  // closes device context - waits for device to disarm
unsigned int Disk_Stream_Detach_Nonblocking      (const char* alias);     // closes device context - pushes close request to a threadpool
unsigned int Disk_Stream_Detach_All_Nonblocking  (void);                  // closes device context - pushes close request to a threadpool
unsigned int Disk_Stream_Attach                  (const char* alias,      // opens device context 
                                                  const char* filename,
                                                  char mode);             //    mode may be 'r' or 'w'
//
// Utilities
//                                                  
unsigned int Disk_Stream_Connect_To_Input        (const char* alias, Device *source_device, int channel);
Device*      Disk_Stream_Get_Device              (const char* alias);

//                                                  
// Tasks
// - different file formats would be implimented through different tasks.
//   The format would then be specified when arming the device.
DeviceTask* Disk_Stream_Create_Raw_Write_Task(void);

DeviceTask* Disk_Stream_Get_Raw_Write_Task(void);
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