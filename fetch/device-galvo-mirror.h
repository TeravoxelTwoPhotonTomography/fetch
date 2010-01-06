#pragma once

#include "util-nidaqmx.h"
#include "device.h"
#include "device-task.h"

#define GALVO_MIRROR_DEFAULT_NAME_LENGTH  32

typedef struct _galvo_mirror_config
{ char task_name          [GALVO_MIRROR_DEFAULT_NAME_LENGTH];
  //DAQmxCreateAOVoltageChan
  char physical_channel   [GALVO_MIRROR_DEFAULT_NAME_LENGTH];
  char channel_label      [GALVO_MIRROR_DEFAULT_NAME_LENGTH];
  float64 min;
  float64 max;
  int32   units;
  //DAQmxCfgSampClkTiming
  char    source          [GALVO_MIRROR_DEFAULT_NAME_LENGTH];
  float64 rate;           /*samples per second per channel*/
  int32   active_edge;
  int32   sample_mode;
  uInt64  samples_per_channel;
  
} Galvo_Mirror_Config;

#define GALVO_MIRROR_CONFIG_EMPTY   {"\0",\
                                     "\0","\0",0,0,DAQmx_Val_Volts,\
                                     "OnboardClock\0",0,DAQmx_Val_Rising,DAQmx_Val_FiniteSamps,0,\
                                    }
#define GALVO_MIRROR_CONFIG_DEFAULT {"galvo-mirror\0",\
                                     "Dev1/ao0\0",\
                                     "galvo-mirror\0",\
                                     -0.5,\
                                      0.5,\
                                     DAQmx_Val_Volts,\
                                     "OnboardClock\0",\
                                     32000,\
                                     DAQmx_Val_Rising,\
                                     DAQmx_Val_ContSamps,\
                                     32000\
                                    }


typedef struct _galvo_mirror
{ TaskHandle          task_handle;
  Galvo_Mirror_Config config;
} Galvo_Mirror;

#define GALVO_MIRROR_EMPTY   {NULL, GALVO_MIRROR_CONFIG_EMPTY}
#define GALVO_MIRROR_DEFAULT {NULL, GALVO_MIRROR_CONFIG_DEFAULT}

//
// Device interface
//

void         Galvo_Mirror_Init                (void);     // Only call once - alloc's a global
unsigned int Galvo_Mirror_Destroy             (void);     // Only call once -  free's a global
unsigned int Galvo_Mirror_Detach              (void);     // closes device context - waits for device to disarm
unsigned int Galvo_Mirror_Detach_Nonblocking  (void);     // closes device context - pushes close request to a threadpool
unsigned int Galvo_Mirror_Attach              (void);     // opens device context

//
// Utilities
//

extern inline  Device*   Galvo_Mirror_Get_Device     (void);

//
// Tasks
//
        DeviceTask* Galvo_Mirror_Create_Task_Continuous_Scan_Immediate_Trigger(void);
        
extern inline  DeviceTask* Galvo_Mirror_Get_Default_Task(void);
//
// Windows
//    testing utilities
//



void             Galvo_Mirror_UI_Append_Menu  ( HMENU hmenu );
void             Galvo_Mirror_UI_Insert_Menu  ( HMENU menu, UINT uPosition, UINT uFlags );
LRESULT CALLBACK Galvo_Mirror_UI_Handler ( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);