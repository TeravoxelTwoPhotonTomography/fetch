#include <NIDAQmx.h>
#include "common.h"

#define NI6713_MAX_SOURCE_FREQUENCY 20000000 // Hz
#define NI6713_SLEW_RATE            20.0     // V/us
#define NI6713_SETTLING_TIME        3.0      // us

#define NI6713_MAX_UPDATE_RATE      1000000  // S/sec
#define NI6713_FIFO_BUFFER_SIZE     16384    // Samples

#define NI6713_MAX_VOLTS_OUT        10       // +/- V
#define NI6713_OUTPUT_IMPEDANCE     0.1      // ohms

//-- 6259

#define NI6529_MAX_SOURCE_FREQUENCY 1000000  // Hz
#define NI6529_SLEW_RATE            20.0     // V/us
#define NI6529_SETTLING_TIME        3.0      // us

#define NI6529_MAX_UPDATE_RATE      1000000  // S/sec
#define NI6529_FIFO_BUFFER_SIZE     4095     // Samples

#define NI6529_MAX_VOLTS_OUT        10       // +/- V
#define NI6529_OUTPUT_IMPEDANCE     0.1      // ohms

int32 Guarded_DAQmx( int32 error, const char* expression, const char* file, const int line, pf_reporter report );
int32 Guarded_DAQmx__Silent_Warnings( int32 error, const char* expression, const char* file, const int line, pf_reporter report );
