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
#define DIGITIZER_DEFAULT_TIMEOUT         100 // ms

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
                       "0-4",\
                       DIGITIZER_MAX_NUM_CHANNELS,\
                       {{"0\0",2.0,NISCOPE_VAL_DC,VI_TRUE},\
                        {"1\0",2.0,NISCOPE_VAL_DC,VI_TRUE},\
                        {"2\0",2.0,NISCOPE_VAL_DC,VI_TRUE},\
                        {"3\0",2.0,NISCOPE_VAL_DC,VI_TRUE},\
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
#define DIGITIZER_DEFUALT {0, DIGITIZER_CONFIG_DEFAULT};

//
// Device interface
//

void         Digitizer_Init    (void);     // Only call once
unsigned int Digitizer_Destroy (void);     // Only call once
unsigned int Digitizer_Close   (void);
unsigned int Digitizer_Off     (void);
unsigned int Digitizer_Hold    (void);

//
// Windows
//    testing utilities
//

#define IDM_DIGITIZER               WM_APP+1
#define IDM_DIGITIZER_OFF           IDM_DIGITIZER+1
#define IDM_DIGITIZER_HOLD          IDM_DIGITIZER+2
#define IDM_DIGITIZER_TASK_STREAM1  IDM_DIGITIZER+3
#define IDM_DIGITIZER_LIST_DEVICES  IDM_DIGITIZER+4
#define IDM_DIGITIZER_TASK_STOP     IDM_DIGITIZER+5

void             Digitizer_UI_Append_Menu  ( HMENU hmenu );
LRESULT CALLBACK Digitizer_UI_Handler ( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);