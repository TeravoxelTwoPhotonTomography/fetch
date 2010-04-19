#pragma once

#include "../agent.h"
#include "../util/util-niscope.h"

#define DIGITIZER_MAX_NUM_CHANNELS        NI5105_MAX_NUM_CHANNELS
#define DIGITIZER_MAX_SAMPLE_RATE         NI5105_MAX_SAMPLE_RATE
#define DIGITIZER_DEVICE_NAME             NI5105_DEVICE_NAME
#define DIGITIZER_DEFAULT_RECORD_LENGTH   512*512
#define DIGITIZER_DEFAULT_RECORD_NUM      1

#define DIGITIZER_BUFFER_NUM_FRAMES       4   // must be a power of two
#define DIGITIZER_DEFAULT_TIMEOUT         INFINITE // ms

#define DIGITIZER_CHANNEL_CONFIG_EMPTY {NULL,0.0,NISCOPE_VAL_DC,VI_FALSE}
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
                        "0\0",\
                        DIGITIZER_MAX_NUM_CHANNELS,\
                        {{"0\0",  0.20,NISCOPE_VAL_DC,VI_TRUE},\
                          {"1\0",  2.0,NISCOPE_VAL_DC,VI_TRUE},\
                          {"2\0",  5.0,NISCOPE_VAL_DC,VI_FALSE},\
                          {"3\0", 10.0,NISCOPE_VAL_DC,VI_FALSE},\
                          {"4\0", 10.0,NISCOPE_VAL_DC,VI_FALSE},\
                          {"5\0", 10.0,NISCOPE_VAL_DC,VI_FALSE},\
                          {"6\0", 10.0,NISCOPE_VAL_DC,VI_FALSE},\
                          {"7\0", 10.0,NISCOPE_VAL_DC,VI_FALSE}}\
                        }
namespace fetch
{
  class Digitizer: public Agent
  {
    public:

      typedef struct _digitizer_channel_config
      { ViChar    *name;     // Null terminated string with channel syntax: e.g. "0-2,7"
        ViReal64   range;    // Volts peak to peak.  (NI-5105) Can be 0.05, 0.2, 1, or 6 V for 50 Ohm.  Resolution is 0.1% dB.
        ViInt32    coupling; // Specifies how to couple the input signal. Refer to NISCOPE_ATTR_VERTICAL_COUPLING for more information.
        ViBoolean  enabled;  // Specifies whether the channel is enabled for acquisition. Refer to NISCOPE_ATTR_CHANNEL_ENABLED for more information.
      } Channel_Config;

      typedef struct _digitizer_config
      { ViChar                  *resource_name;       // NI device name: e.g. "Dev6"
        ViReal64                 sample_rate;         // samples/second
        ViInt32                  record_length;       // samples per scan
        ViInt32                  num_records;         // number of records per acquire call.
        ViReal64                 reference_position;  // as a percentage
        ViChar                  *acquisition_channels;// the channels to acquire data on (NI Channel String syntax)
        ViInt32                  num_channels;        // number of channels to independantly configure - typically set to DIGITIZER_MAX_NUM_CHANNELS
        Channel_Config           channels[DIGITIZER_MAX_NUM_CHANNELS]; // array of channel configurations  
      } Config;

      ViSession vi;
      Config    config;
    public:
      Digitizer();

      unsigned int attach(void);
      unsigned int detach(void);
  };

} // namespace fetch
