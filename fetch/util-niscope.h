//TODO: push/pop state for niscope?

#pragma once
#include "niscope.h"

#define NI5105_MAX_NUM_CHANNELS 8
#define NI5105_MAX_SAMPLE_RATE  60000000.0
#define NI5105_DEVICE_NAME      "Dev2\0"    // TODO: autodetect this

ViStatus niscope_chk ( ViSession vi, 
                       ViStatus result, 
                       const char *expression,
                       pf_reporter report);

void niscope_log_wfminfo ( pf_reporter      pfOutput, 
                           niScope_wfmInfo *info );

void niscope_debug_list_devices(void);