//
// Device interface
//

void         Video_Display_Init                (void);     // Only call once - alloc's a global
unsigned int Video_Display_Destroy             (void);     // Only call once -  free's a global
unsigned int Video_Display_Detach              (void);     // closes device context - waits for device to disarm
unsigned int Video_Display_Detach_Nonblocking  (void);     // closes device context - pushes close request to a threadpool
unsigned int Video_Display_Attach              (void);     // opens device context

//
// Tasks
//
DeviceTask* Video_Display_Task(void);

//
// Windows
//    testing utilities
//

#define IDM_VIDEO_DISPLAY               WM_APP+30
#define IDM_VIDEO_DISPLAY_DETACH        IDM_VIDEO_DISPLAY+1
#define IDM_VIDEO_DISPLAY_ATTACH        IDM_VIDEO_DISPLAY+2
#define IDM_VIDEO_DISPLAY_LIST_DEVICES  IDM_VIDEO_DISPLAY+3
#define IDM_VIDEO_DISPLAY_TASK_STOP     IDM_VIDEO_DISPLAY+4
#define IDM_VIDEO_DISPLAY_TASK_RUN      IDM_VIDEO_DISPLAY+5

#define IDM_VIDEO_DISPLAY_TASK_0        IDM_VIDEO_DISPLAY+10

void             Video_Display_UI_Append_Menu  ( HMENU hmenu );
void             Video_Display_UI_Insert_Menu  ( HMENU menu, UINT uPosition, UINT uFlags );
LRESULT CALLBACK Video_Display_UI_Handler ( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);