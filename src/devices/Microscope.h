/*
 * Microscope.h
 *
 * Author: Nathan Clack <clackn@janelia.hhmi.org>
 *   Date: Apr 28, 2010
 */
/*
 * Copyright 2010 Howard Hughes Medical Institute.
 * All rights reserved.
 * Use is subject to Janelia Farm Research Campus Software Copyright 1.1
 * license terms (http://license.janelia.org/license/jfrc_copyright_1_1.html).
 */
#pragma once

#include "agent.h"
#include "task.h"
#include "scanner3D.h"
#include "microscope.pb.h"

#include "workers/FrameAverage.h"
#include "workers/HorizontalDownsampler.h"
#include "workers/Terminator.h"
#include "workers/FrameCaster.h"
#include "workers/ResonantWrap.h"
#include "workers/FrameInvert.h"

#include "DiskStream.h"
#include <string>
#include "tasks/microscope-interaction.h"

#define MICROSCOPE_DEFAULT_DEST_PATH           "E:\\microscope\\data\\2010-07-27\\4\\"
#define MICROSCOPE_DEFAULT_FILE_NAME_TEMPLATE  "Stack_%03d.stream"
#define MICROSCOPE_DEFAULT_INITIAL_FILE_ID     0

#define MICROSCOPE_MAX_WORKERS     10
#define MICROSCOPE_DEFAULT_TIMEOUT INFINITE


// forward declare
namespace fetch {
  namespace task {
    namespace microscope {
      class Interaction;
    }
  }
}

namespace fetch
{ namespace device
  {
  
//TODO: move this into it's own header and out of namespace fetch::device (move to fetch::)
    class FileSeries
    {
    public:
      FileSeries() :_desc(&__default_desc), _lastpath("lastpath") {}
      FileSeries(cfg::FileSeries *desc) :_desc(desc), _lastpath("lastpath") {Guarded_Assert(_desc!=NULL);}

      FileSeries& inc(void);
      const std::string getFullPath(const std::string& shortname);
      void ensurePathExists();

    private:
      void renderSeriesNo( char * strSeriesNo, int maxbytes );
      void tryCreateDirectory( LPCTSTR root_date, const char* description, LPCTSTR root );
      void updateDate(void);
      std::string _lastpath;
      cfg::FileSeries __default_desc;

    public:
      cfg::FileSeries *_desc;
    };    

    class Microscope : public IConfigurableDevice<cfg::device::Microscope>
    { 
    public:     
      // Will attach and arm the default task on construction
      Microscope();
      Microscope(const Config &cfg);
      Microscope(Config *cfg);

      void __common_setup();

      ~Microscope();

      unsigned int attach(void);
      unsigned int detach(void);

      unsigned int disarm();

      virtual void _set_config(Config IN *cfg);

      IDevice* configPipeline();  // returns the end of the pipeline

      inline IDevice* pipelineEnd() {return &wrap;}
    public:
      const std::string next_filename(); // increment the file series counter

    public:
      device::Scanner3D              scanner;

      worker::FrameAverageAgent 	   frame_averager;
      worker::HorizontalDownsampleAgent pixel_averager;
      worker::FrameCastAgent_i16     cast_to_i16;
      worker::FrameInvertAgent       inverter;
      worker::ResonantWrapAgent      wrap;

      worker::TerminalAgent		       trash;                
      device::DiskStreamMessage      disk;

      task::microscope::Interaction  interaction_task;      

    public:
      FileSeries file_series;

    public:
      Agent __self_agent;
      Agent __scan_agent;
      Agent __io_agent;
    };
    //end namespace fetch::device
  }
}
