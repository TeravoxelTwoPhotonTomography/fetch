//TODO: push/pop state for niscope?

#pragma once
#include "config.h"
#ifdef HAVE_NISCOPE
#include "niscope.h"

#define NI5105_MAX_NUM_CHANNELS 8
#define NI5105_MAX_SAMPLE_RATE  60000000.0
#define NI5105_DEVICE_NAME      "Dev3\0"    // TODO: autodetect this

ViStatus niscope_chk ( ViSession vi, 
                       ViStatus result, 
                       const char *expression,
                       const char *file,
                       const int line,
                       pf_reporter report);

void niscope_log_wfminfo ( pf_reporter      pfOutput, 
                           niScope_wfmInfo *info );

void niscope_debug_list_devices(void);

void niscope_debug_print_status( ViSession vi );

void niscope_cfg_rtsi_default( ViSession vi );

double niscope_get_backlog( ViSession vi );

template<class TPixel> 
ViStatus Fetch (ViSession vi,
                ViConstString channellist,
                ViReal64 timeout,
                ViInt32 numsamples,
                TPixel* data,
                struct niScope_wfmInfo *info);
#endif //HAVE_NISCOPE