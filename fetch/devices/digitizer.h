/*
 * Digitizer.h
 *
 * Author: Nathan Clack <clackn@janelia.hhmi.org>
 *   Date: Apr 20, 2010
 */
/*
 * Copyright 2010 Howard Hughes Medical Institute.
 * All rights reserved.
 * Use is subject to Janelia Farm Research Campus Software Copyright 1.1
 * license terms (http://license.janelia.org/license/jfrc_copyright_1_1.html).
 */
/*
 * Notes
 * -----
 *
 * In niScope, a waveform is a single channel for a single record.  A
 * multi-record acquisition is used when one needs fast retriggering.  The data
 * in a record is formated:
 *
 *   [--- chan 0 ---] [--- chan 1 ---] ... [--- chan N ---] : record k
 *
 * For a multirecord acquisition:
 *
 *   [--- record 1 ---][--- record 2 ---] ...
 *
 * So, we have this identity:
 *
 *   #waveforms = #records x #channels
 */
#pragma once

#include "../agent.h"
#include "../util/util-niscope.h"

#define DIGITIZER_MAX_NUM_CHANNELS        NI5105_MAX_NUM_CHANNELS
#define DIGITIZER_MAX_SAMPLE_RATE        (NI5105_MAX_SAMPLE_RATE)
#define DIGITIZER_DEVICE_NAME             NI5105_DEVICE_NAME
#define DIGITIZER_DEFAULT_RECORD_LENGTH   512*512
#define DIGITIZER_DEFAULT_RECORD_NUM      1

#define DIGITIZER_BUFFER_NUM_FRAMES       4        // must be a power of two
#define DIGITIZER_DEFAULT_TIMEOUT         INFINITE // ms

namespace fetch
{
  namespace device
  {
    class Digitizer : public virtual Agent
    {
    public:
      Digitizer();
      Digitizer(size_t nbuf, size_t nbytes_per_frame, size_t nwfm); // Inputs determine how output queues are initially allocated
      ~Digitizer();

      unsigned int attach(void);
      unsigned int detach(void);
      unsigned int is_attached(void);

    public:
      struct Channel_Config
      {
        ViChar   *name;    // Null terminated string with channel syntax: e.g. "0-2,7"
        ViReal64  range;   // Volts peak to peak.  (NI-5105) Can be 0.05, 0.2, 1, or 6 V for 50 Ohm.  Resolution is 0.1% dB.
        ViInt32   coupling;// Specifies how to couple the input signal. Refer to NISCOPE_ATTR_VERTICAL_COUPLING for more information.
        ViBoolean enabled; // Specifies whether the channel is enabled for acquisition. Refer to NISCOPE_ATTR_CHANNEL_ENABLED for more information.
        
        Channel_Config()
        : name(NULL),
          range(0.0),
          coupling(NISCOPE_VAL_DC),
          enabled(VI_FALSE)
          {}
        
        Channel_Config(ViChar *name,ViReal64 range,ViInt32 coupling,ViBoolean enabled)
        : name(name),
          range(range),
          coupling(coupling),
          enabled(enabled)
          {}
      };

      struct Config
      {
        ViChar        *resource_name;                        // NI device name: e.g. "Dev6"
        ViReal64       sample_rate;                          // samples/second
        ViInt32        record_length;                        // samples per scan
        ViInt32        num_records;                          // number of records per acquire call.
        ViReal64       reference_position;                   // as a percentage
        ViChar        *acquisition_channels;                 // the channels to acquire data on (NI Channel String syntax)
        ViInt32        num_channels;                         // number of channels to independently configure - typically set to DIGITIZER_MAX_NUM_CHANNELS
        Channel_Config channels[DIGITIZER_MAX_NUM_CHANNELS]; // array of channel configurations
        
        Config()
        : resource_name         (DIGITIZER_DEVICE_NAME),
          sample_rate           (DIGITIZER_MAX_SAMPLE_RATE),
          record_length         (DIGITIZER_DEFAULT_RECORD_LENGTH),
          num_records           (DIGITIZER_DEFAULT_RECORD_NUM),
          reference_position    (0.0),
          acquisition_channels  ("0\0"), 
          num_channels          (DIGITIZER_MAX_NUM_CHANNELS)
        { channels[0] = Channel_Config("0\0",  0.20,NISCOPE_VAL_DC,VI_TRUE);
          channels[1] = Channel_Config("1\0",  2.0 ,NISCOPE_VAL_DC,VI_TRUE);
          channels[2] = Channel_Config("2\0",  5.0 ,NISCOPE_VAL_DC,VI_FALSE);
          channels[3] = Channel_Config("3\0", 10.0 ,NISCOPE_VAL_DC,VI_FALSE);
          channels[4] = Channel_Config("4\0", 10.0 ,NISCOPE_VAL_DC,VI_FALSE);
          channels[5] = Channel_Config("5\0", 10.0 ,NISCOPE_VAL_DC,VI_FALSE);
          channels[6] = Channel_Config("6\0", 10.0 ,NISCOPE_VAL_DC,VI_FALSE);
          channels[7] = Channel_Config("7\0", 10.0 ,NISCOPE_VAL_DC,VI_FALSE);
        }
      };

      ViSession vi;
      Config    config;
    };
  }
} // namespace fetch
