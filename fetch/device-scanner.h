#pragma once

#include "device.h"

//
// Device configuration 
//    definitions
//    defaults
//

#define SCANNER_DEFAULT_RESONANT_FREQUENCY    7920.0   // Hz
#define SCANNER_DEFAULT_SCANS                 512      // Number of full resonant periods that make up a frame
                                                       //        image height = 2 x scans
#define SCANNER_DEFAULT_LINE_DUTY_CYCLE       0.95     // Fraction of resonant period to acquire (must be less than one)
#define SCANNER_DEFAULT_GALVO_SAMPLES         4096     // samples per waveform
#define SCANNER_DEFAULT_GALVO_VPP              5.0     // V - peak-to-peak
#define SCANNER_DEFAULT_GALVO_V_MAX           10.0     // V - peak-to-peak
#define SCANNER_DEFAULT_GALVO_V_MIN          -10.0     // V - peak-to-peak
#define SCANNER_DEFAULT_GALVO_CHANNE    "Dev1/ao0"     // DAQ terminal: should be connected to command input on galvo controller
#define SCANNER_DEFAULT_GALVO_TRIGGER      "APFI0"     // DAQ terminal: should be connected to resonant velocity output
#define SCANNER_DEFAULT_GALVO_ARMSTART      "PFI0"     // DAQ terminal: should be connected to "ReadyForStart" event output from digitizer
#define SCANNER_DEFAULT_GALVO_CLOCK    "Dev1/ctr1"     // DAQ terminal: used to produce an appropriately triggered set of pulses as ao sample clock

#define SCANNER_DEFAULT_TIMEOUT               INFINITE // ms
#define SCANNER_MAX_CHAN_STRING                     32 // characters

typedef struct _scanner_config
{
  f64         resonant_frequency;
  u32         scans;
  f64         line_duty_cycle;
  u32         galvo_samples;
  f64         galvo_vpp;
  f64         galvo_v_max;
  f64         galvo_v_min;
  char        galvo_channel [SCANNER_MAX_CHAN_STRING];
  char        galvo_trigger [SCANNER_MAX_CHAN_STRING];
  char        galvo_armstart[SCANNER_MAX_CHAN_STRING];
  char        galvo_clock   [SCANNER_MAX_CHAN_STRING];
} Scanner_Config;

#define SCANNER_CONFIG_EMPTY \
                      { SCANNER_DEFAULT_RESONANT_FREQUENCY,\
                        SCANNER_DEFAULT_SCANS,\
                        SCANNER_DEFAULT_LINE_DUTY_CYCLE,\
                        SCANNER_DEFAULT_GALVO_SAMPLES,\
                        SCANNER_DEFAULT_GALVO_VPP,\
                        SCANNER_DEFAULT_GALVO_V_MAX,\
                        SCANNER_DEFAULT_GALVO_V_MIN,\
                        SCANNER_DEFAULT_GALVO_CHANNEL,\
                        SCANNER_DEFAULT_GALVO_TRIGGER,\
                        SCANNER_DEFAULT_GALVO_ARMSTART,\
                        SCANNER_DEFAULT_GALVO_CLOCK\
                      }

typedef struct _scanner 
{ Digitizer     *digitizer;
  TaskHandle     daq_ao;
  TaskHandle     daq_clk;
  Scanner_Config config;
} Scanner;

#define SCANNER_EMPTY   {NULL, NULL, NULL, SCANNER_CONFIG_EMPTY};
#define SCANNER_DEFAULT {NULL, NULL, NULL, SCANNER_CONFIG_DEFAULT};

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

//
// Windows
//    UI utilities
//

void             Scanner_UI_Append_Menu  ( HMENU hmenu );
void             Scanner_UI_Insert_Menu  ( HMENU menu, UINT uPosition, UINT uFlags );
LRESULT CALLBACK Scanner_UI_Handler      ( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

