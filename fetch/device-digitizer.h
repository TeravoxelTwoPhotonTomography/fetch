#pragma once

#include "device.h"
#include "util-niscope.h"

//
// Device configuration 
//    definitions
//    defaults
//

#define DIGITIZER_MAX_NUM_CHANNELS        NI5105_MAX_NUM_CHANNELS
#define DIGITIZER_MAX_SAMPLE_RATE         NI5105_MAX_SAMPLE_RATE
#define DIGITIZER_DEVICE_NAME             NI5105_DEVICE_NAME
#define DIGITIZER_DEFAULT_RECORD_LENGTH   1024
#define DIGITIZER_DEFAULT_RECORD_NUM      1

#define DIGITIZER_BUFFER_NUM_FRAMES       64  // must be a power of two
#define DIGITIZER_DEFAULT_TIMEOUT         INFINITE // ms

typedef struct _digitizer_channel_config
{ ViChar    *name;     // Null terminated string with channel syntax: e.g. "0-2,7"
  ViReal64   range;    // Volts peak to peak
  ViInt32    coupling; // Specifies how to couple the input signal. Refer to NISCOPE_ATTR_VERTICAL_COUPLING for more information.
  ViBoolean  enabled;  // Specifies whether the channel is enabled for acquisition. Refer to NISCOPE_ATTR_CHANNEL_ENABLED for more information.
} Digitizer_Channel_Config;

#define DIGITIZER_CHANNEL_CONFIG_EMPTY {NULL,0.0,NISCOPE_VAL_DC,VI_FALSE}

typedef struct _digitizer_config
{ ViChar                  *resource_name;       // NI device name: e.g. "Dev6"
  ViReal64                 sample_rate;         // samples/second
  ViInt32                  record_length;       // samples per scan
  ViInt32                  num_records;         // number of records per acquire call.
  ViReal64                 reference_position;  // as a percentage
  ViChar                  *acquisition_channels;// the channels to acquire data on (NI Channel String syntax)
  ViInt32                  num_channels;        // number of channels to independantly configure
  Digitizer_Channel_Config channels[DIGITIZER_MAX_NUM_CHANNELS]; // array of channel configurations  
} Digitizer_Config;

#define DIGITIZER_CONFIG_EMPTY \
                     {NULL,\
                      0.0,\
                        0,\
                        0,\
                      0.0,\
                      " ",\
                      DIGITIZER_MAX_NUM_CHANNELS,\
                      {DIGITIZER_CHANNEL_CONFIG_EMPTY,\
                       DIGITIZER_CHANNEL_CONFIG_EMPTY,\
                       DIGITIZER_CHANNEL_CONFIG_EMPTY,\
                       DIGITIZER_CHANNEL_CONFIG_EMPTY,\
                       DIGITIZER_CHANNEL_CONFIG_EMPTY,\
                       DIGITIZER_CHANNEL_CONFIG_EMPTY,\
                       DIGITIZER_CHANNEL_CONFIG_EMPTY,\
                       DIGITIZER_CHANNEL_CONFIG_EMPTY}\
                      }

#define DIGITIZER_CONFIG_DEFAULT \
                     { DIGITIZER_DEVICE_NAME,\
                       DIGITIZER_MAX_SAMPLE_RATE,\
                       DIGITIZER_DEFAULT_RECORD_LENGTH,\
                       DIGITIZER_DEFAULT_RECORD_NUM,\
                       0.0,\
                       "0",\
                       DIGITIZER_MAX_NUM_CHANNELS,\
                       {{"0\0",2.0,NISCOPE_VAL_DC,VI_TRUE},\
                        {"1\0",2.0,NISCOPE_VAL_DC,VI_FALSE},\
                        {"2\0",2.0,NISCOPE_VAL_DC,VI_FALSE},\
                        {"3\0",2.0,NISCOPE_VAL_DC,VI_FALSE},\
                        {"4\0",2.0,NISCOPE_VAL_DC,VI_FALSE},\
                        {"5\0",2.0,NISCOPE_VAL_DC,VI_FALSE},\
                        {"6\0",2.0,NISCOPE_VAL_DC,VI_FALSE},\
                        {"7\0",2.0,NISCOPE_VAL_DC,VI_FALSE}}\
                       }

typedef struct _digitizer 
{ ViSession           vi;
  Digitizer_Config    config;
} Digitizer;

#define DIGITIZER_EMPTY   {0, DIGITIZER_CONFIG_EMPTY};
#define DIGITIZER_DEFAULT {0, DIGITIZER_CONFIG_DEFAULT};

//
// Device interface
//

void         Digitizer_Init                (void);     // Only call once - alloc's a global
unsigned int Digitizer_Destroy             (void);     // Only call once -  free's a global
unsigned int Digitizer_Detach              (void);     // closes device context - waits for device to disarm
unsigned int Digitizer_Detach_Nonblocking  (void);     // closes device context - pushes close request to a threadpool
unsigned int Digitizer_Attach              (void);     // opens device context

//
// Utilities
//
Device *Digitizer_Get_Device(void);

//
// Tasks
//
DeviceTask* Digitizer_Create_Task_Stream_All_Channels_Immediate_Trigger(void);


DeviceTask* Digitizer_Get_Default_Task(void);
//
// Windows
//    testing utilities
//

#define IDM_DIGITIZER               WM_APP+1
#define IDM_DIGITIZER_DETACH        IDM_DIGITIZER+1
#define IDM_DIGITIZER_ATTACH        IDM_DIGITIZER+2
#define IDM_DIGITIZER_LIST_DEVICES  IDM_DIGITIZER+3
#define IDM_DIGITIZER_TASK_STOP     IDM_DIGITIZER+4
#define IDM_DIGITIZER_TASK_RUN      IDM_DIGITIZER+5

#define IDM_DIGITIZER_TASK_0        IDM_DIGITIZER+10

void             Digitizer_UI_Append_Menu  ( HMENU hmenu );
void             Digitizer_UI_Insert_Menu  ( HMENU menu, UINT uPosition, UINT uFlags );
LRESULT CALLBACK Digitizer_UI_Handler ( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);