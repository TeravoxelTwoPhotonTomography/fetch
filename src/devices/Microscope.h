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
#include "microscope.pb.h"

#include "workers/FrameAverage.h"
#include "workers/HorizontalDownsampler.h"
#include "workers/Terminator.h"
#include "workers/FrameCaster.h"
#include "workers/ResonantWrap.h"
#include "workers/ResonantUnwarp.h"
#include "workers/FrameInvert.h"
#include "workers/FrameFormater.h"

#include "devices/scanner3D.h"
#include "devices/DiskStream.h"
#include "devices/LinearScanMirror.h"
#include "devices/pockels.h"
#include "devices/Stage.h"
#include "devices/FieldOfViewGeometry.h"
#include "devices/Vibratome.h"
#include "tasks/microscope-interaction.h"
#include "tasks/StackAcquisition.h"
#include "tasks/TiledAcquisition.h"  
#include "tasks/Vibratome.h"

#include <string>
#include <set>

#define MICROSCOPE_MAX_WORKERS     10
#define MICROSCOPE_DEFAULT_TIMEOUT INFINITE


namespace fetch
{ namespace device
  {
  
//TODO: move this into it's own header and out of namespace fetch::device (move to fetch::)
    struct FileSeriesListener
    { virtual void update(const std::string& path) = 0;
    };

    class FileSeries
    {
      typedef std::set<FileSeriesListener*> TListeners;
    public:
      FileSeries() :_desc(&__default_desc), _lastpath("lastpath"), _is_valid(false), _prev(__default_desc)  {_lastpath=_desc->root() + _desc->pathsep() + _desc->date();}
      FileSeries(cfg::FileSeries *desc) :_desc(desc), _lastpath("lastpath"), _is_valid(false)               {Guarded_Assert(_desc!=NULL);_lastpath=_desc->root() + _desc->pathsep() + _desc->date(); _prev.CopyFrom(*_desc);}

      FileSeries& inc(void);
      const std::string getFullPath(const std::string& prefix, const std::string& ext);
      const std::string getPath();
      bool updateDesc(cfg::FileSeries *desc);
      bool ensurePathExists();
      inline bool is_valid()  {return _is_valid;};

      void addListener(FileSeriesListener *x) {_listeners.insert(x);}

    private:
      void renderSeriesNo( char * strSeriesNo, int maxbytes );
      void tryCreateDirectory( LPCTSTR root_date, const char* description, LPCTSTR root );
      void updateDate(void);
      std::string _lastpath;
      cfg::FileSeries __default_desc;
      bool _is_valid;

      void notify();

    public: // pseudo-private
      cfg::FileSeries *_desc;
      cfg::FileSeries  _prev;      
      TListeners _listeners;
    };    

    class Microscope : public IConfigurableDevice<cfg::device::Microscope>
    { 
    public:     
      // Will on_attach and arm the default task on construction
      Microscope();
      Microscope(const Config &cfg);
      Microscope(Config *cfg);

      void __common_setup();

      ~Microscope();

      unsigned int on_attach(void);
      unsigned int on_detach(void);

      unsigned int on_disarm();

      virtual void _set_config(Config IN *cfg);
      virtual void _set_config(const Config& cfg);
      virtual void onUpdate();

      IDevice* configPipeline();                                           // returns the end of the pipeline
      unsigned int runPipeline();
      unsigned int stopPipeline();

      inline IDevice* pipelineEnd() {return &wrap;}

    public:
      const std::string stack_filename();                                  // get the current file
      const std::string config_filename();                                 // get the current file
      const std::string metadata_filename();       
                   void write_stack_metadata();     

    public:
      device::Scanner3D                  scanner;
      device::Stage                      stage_;
      device::Vibratome                  vibratome_;
      device::FieldOfViewGeometry        fov_;

      worker::FrameAverageAgent 	       frame_averager;
      worker::HorizontalDownsampleAgent  pixel_averager;
      worker::FrameCastAgent_i16         cast_to_i16;
      worker::FrameInvertAgent           inverter;
      worker::ResonantWrapAgent          wrap;
      worker::ResonantUnwarpAgent        unwarp;
      worker::FrameFormatterAgent        frame_formatter;

      worker::TerminalAgent		           trash;
      device::TiffStream                 disk;
                                                       
      task::microscope::Interaction      interaction_task;
      task::microscope::StackAcquisition stack_task;      
      task::microscope::TiledAcquisition tiling_task;
      task::microscope::Cut              cut_task;

      inline Chan*  getVideoChannel()    {if(unwarp._out) return unwarp._out->contents[0]; else return NULL;}
      inline LinearScanMirror*  LSM()    {return &scanner._scanner2d._LSM;}
      inline Pockels*       pockels()    {return &scanner._scanner2d._pockels;}
      inline ZPiezo*         zpiezo()    {return &scanner._zpiezo;}
      inline Stage*           stage()    {return &stage_;}
      inline Vibratome*   vibratome()    {return &vibratome_;}

      int updateFovFromStackDepth(int nowait=0);  // These also account for cut thickness, returns 0 if overlap is not positive.
      int updateStackDepthFromFov(int nowait=0);

    public:
      FileSeries file_series;

    public:
      Agent __self_agent;
      Agent __scan_agent;
      Agent __io_agent;
      Agent __vibratome_agent;
    };
    //end namespace fetch::device
  }
}
