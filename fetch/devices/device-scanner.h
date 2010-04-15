#pragma once

#include "device.h"
#include "device-digitizer.h"
#include "util-nidaqmx.h"

//
// Device configuration 
//    definitions
//    defaults
//
#define SCANNER_QUEUE_NUM_FRAMES                   32

#define SCANNER_DEFAULT_RESONANT_FREQUENCY         7920.0 // Hz
#define SCANNER_DEFAULT_SCANS                      512    // Number of full resonant periods that make up a frame
                                                          //        image height = 2 x scans
#define SCANNER_DEFAULT_LINE_DUTY_CYCLE           0.95f   // Fraction of resonant period to acquire (must be less than one)
#define SCANNER_DEFAULT_LINE_TRIGGER_SRC              1   // Digitizer channel corresponding to resonant velocity input
                                                          // the channel should be appropriately configured in the digitizer config
#define SCANNER_DEFAULT_GALVO_SAMPLES              4096   // samples per waveform
#define SCANNER_DEFAULT_GALVO_VPP                  10.0    // V - peak-to-peak
#define SCANNER_DEFAULT_GALVO_V_MAX                10.0   // V - Maximum permissible value
#define SCANNER_DEFAULT_GALVO_V_MIN               -10.0   // V - Minimum permissible value
#define SCANNER_DEFAULT_GALVO_CHANNEL        "/Dev1/ao0"  // DAQ terminal: should be connected to command input on galvo controller
#define SCANNER_DEFAULT_GALVO_TRIGGER            "APFI0"  // DAQ terminal: should be connected to resonant velocity output
#define SCANNER_DEFAULT_GALVO_ARMSTART           "RTSI2"  // DAQ terminal: should be connected to "ReadyForStart" event output from digitizer
#define SCANNER_DEFAULT_GALVO_CLOCK "Ctr1InternalOutput"  // DAQ terminal: used to produce an appropriately triggered set of pulses as ao sample clock
#define SCANNER_DEFAULT_GALVO_CTR           "/Dev1/ctr1"  // DAQ terminal: used to produce an appropriately triggered set of pulses as ao sample clock

#define SCANNER_DEFAULT_POCKELS_V_MAX                2.0
#define SCANNER_DEFAULT_POCKELS_V_MIN                0.0
#define SCANNER_DEFAULT_POCKELS_V_OPEN               0.0  // The value to which the pockels cell will be set during a frame.  Default should be a safe value.
#define SCANNER_DEFAULT_POCKELS_V_CLOSED             0.0  // The value to which the pockels cell will be set between frames.
#define SCANNER_DEFAULT_POCKELS_AO_CHANNEL    "/Dev1/ao2"
#define SCANNER_DEFAULT_POCKELS_AI_CHANNEL   "/Dev1/ai16"

#define SCANNER_DEFAULT_SHUTTER_CHANNEL "/Dev1/port0/line8"
#define SCANNER_DEFAULT_SHUTTER_OPEN                     0
#define SCANNER_DEFAULT_SHUTTER_CLOSED                   1

#define SCANNER_DEFAULT_TIMEOUT               INFINITE // ms
#define SCANNER_MAX_CHAN_STRING                     32 // characters

typedef struct _scanner_config
{
  f64         resonant_frequency;
  u32         scans;
  f32         line_duty_cycle;
  u8          line_trigger_src;
  
  u32         galvo_samples;
  f64         galvo_vpp;
  f64         galvo_v_lim_max;
  f64         galvo_v_lim_min;
  char        galvo_channel [SCANNER_MAX_CHAN_STRING];
  char        galvo_trigger [SCANNER_MAX_CHAN_STRING];
  char        galvo_armstart[SCANNER_MAX_CHAN_STRING];
  char        galvo_clock   [SCANNER_MAX_CHAN_STRING];
  char        galvo_ctr     [SCANNER_MAX_CHAN_STRING];
  
  f64         pockels_v_max;
  f64         pockels_v_min;
  f64         pockels_v_open;
  f64         pockels_v_closed;
  char        pockels_ao_chan[SCANNER_MAX_CHAN_STRING];
  char        pockels_ai_chan[SCANNER_MAX_CHAN_STRING];
} Scanner_Config;

#define SCANNER_CONFIG_DEFAULT \
                      { SCANNER_DEFAULT_RESONANT_FREQUENCY,\
                        SCANNER_DEFAULT_SCANS,\
                        SCANNER_DEFAULT_LINE_DUTY_CYCLE,\
                        SCANNER_DEFAULT_LINE_TRIGGER_SRC,\
                        SCANNER_DEFAULT_GALVO_SAMPLES,\
                        SCANNER_DEFAULT_GALVO_VPP,\
                        SCANNER_DEFAULT_GALVO_V_MAX,\
                        SCANNER_DEFAULT_GALVO_V_MIN,\
                        SCANNER_DEFAULT_GALVO_CHANNEL "," SCANNER_DEFAULT_POCKELS_AO_CHANNEL,\
                        SCANNER_DEFAULT_GALVO_TRIGGER,\
                        SCANNER_DEFAULT_GALVO_ARMSTART,\
                        SCANNER_DEFAULT_GALVO_CLOCK,\
                        SCANNER_DEFAULT_GALVO_CTR,\
                        SCANNER_DEFAULT_POCKELS_V_MAX,\
                        SCANNER_DEFAULT_POCKELS_V_MIN,\
                        SCANNER_DEFAULT_POCKELS_V_OPEN,\
                        SCANNER_DEFAULT_POCKELS_V_CLOSED,\
                        SCANNER_DEFAULT_POCKELS_AO_CHANNEL,\
                        SCANNER_DEFAULT_POCKELS_AI_CHANNEL,\
                      }

typedef struct _scanner 
{ Digitizer     *digitizer;
  TaskHandle     daq_ao;
  TaskHandle     daq_clk;
  TaskHandle     daq_shutter;
  Scanner_Config config;
} Scanner;

#define SCANNER_DEFAULT {NULL, NULL, NULL, NULL, SCANNER_CONFIG_DEFAULT};

//
// Device interface
//

void         Scanner_Init                (void);     // Only call once - alloc's a global
unsigned int Scanner_Destroy             (void);     // Only call once -  free's a global
unsigned int Scanner_Detach              (void);     // closes device context - waits for device to disarm
unsigned int Scanner_Detach_Nonblocking  (void);     // closes device context - pushes close request to a threadpool
unsigned int Scanner_Attach              (void);     // opens device context

//
// Utilities
//
extern inline Scanner*    Scanner_Get                 (void);
extern inline Device*     Scanner_Get_Device          (void);
extern inline DeviceTask* Scanner_Get_Default_Task    (void);

              int         Scanner_Pockels_Is_Volts_In_Bounds      ( f64 volts );
              int         Scanner_Pockels_Set_Open_Val            ( f64 volts, int time ); // The time argument is used to synchronize "simultaneous" requests. Returns the last timestamp.
              BOOL        Scanner_Pockels_Set_Open_Val_Nonblocking( f64 volts );

//
// Windows
//    UI utilities
//

void             Scanner_UI_Append_Menu  ( HMENU hmenu );
void             Scanner_UI_Insert_Menu  ( HMENU menu, UINT uPosition, UINT uFlags );
LRESULT CALLBACK Scanner_UI_Handler      ( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);