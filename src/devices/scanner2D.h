/*
 * Scanner2D.h
 *
 *  Created on: Apr 19, 2010
 *      Author: Nathan Clack <clackn@janelia.hhmi.org>
 */
/*
 * Copyright 2010 Howard Hughes Medical Institute.
 * All rights reserved.
 * Use is subject to Janelia Farm Research Campus Software Copyright 1.1
 * license terms (http://license.janelia.org/license/jfrc_copyright_1_1.html).
 */
/*
 * Scanner2D
 * ---------
 *
 * This class models a device capable of point scanned image acquisition using
 * a resonant mirror on one axis.
 *
 * Acquisition using a resonant mirror requires synchronizing multiple devices.
 * Here, I've broken the operation of the scanner into four parts, each
 * represented by parent classes.  The parent classes are all eventually
 * virtually derived from the Agent class, so Agent state is shared among all
 * the parent classes.  The hierarchy looks like this:
 *
 * (Agent)-------|--------------|
 *           Digitizer      NIDAQAgent----|-------|--------------|
 *               |                     Pockels  Shutter  LinearScanMirror
 *                \                      /       /              /
 *                 \____________________/_______/______________/
 *                           |
 *                        Scanner2D
 *
 * The NIDAQAgent and Digitizer classes both provide the attach/detach
 * functions required of a class implementing the Agent interface.
 * The NIDAQAgent class is _not_ virtually inherited so each of the Pockels,
 * Shutter, and LinearScanMirror classes bring in a hardware context handle:
 * <daqtask>.  The handles all have the same name, so to unambiguously access
 * the <daqtask> member, a down cast is required.
 *
 * It's possible to get independent task handles for the pockels cell, the
 * shutter and the linear scan mirror by calling the attach() function for each.
 * Here, however, the Pockels cell and linear scan mirror should be associated
 * with the same analog output task.  This is done by keeping attaching to
 * a separate task stored by the Scanner2D class; the <daqtask> handles for
 * the Pockels and LinearScanMirror classes go unused.
 *
 * Note that Pockels and LinearScanMirror interfaces both implement hardware
 * changes by (a) updating their associated software state and then, if armed,
 * (b) committing changes to hardware via the associated Task.  Because hardware
 * commits are routed through the Task, the specific <daqtask> used is up to the
 * Task. The Shutter class, does _not_ follow this pattern.  In it's current
 * form, Shutter::attach() _must_ be used in order to use Shutter::Open() and
 * Shutter::Closed().
 *
 * Clear as Mud.
 */
#pragma once
#include "digitizer.h"
#include "pockels.h"
#include "shutter.h"
#include "LinearScanMirror.h"
#include "../frame.h"
#include "scanner2d.pb.h"
#include "object.h"

#define SCANNER2D_DEFAULT_TIMEOUT               INFINITE // ms

namespace fetch
{
  namespace device
  {

    class Scanner2D : public Digitizer, 
                      public Pockels, 
                      public Shutter, 
                      public LinearScanMirror,
                      public Configurable<cfg::device::Scanner2D>
    {
    public:
      typedef cfg::device::Scanner2D Config;            // Need to declare these here to disambiguate.
      Config* &config;      

      Scanner2D();
      Scanner2D(Config *cfg);

      virtual ~Scanner2D();

      unsigned int attach(void);                         // Returns 0 on success, 1 otherwise
      unsigned int detach(void);                         // Returns 0 on success, 1 otherwise

      virtual void set_config(Config *cfg);
    public:
      TaskHandle  ao,
                  clk;
      HANDLE      notify_daq_done;
      vector_f64 *ao_workspace;

    public: //Section: protected with Tasks as friends.
      ViInt32                      _compute_record_size(void);   // determines the number of elements acquired with each digitizer record
      Frame_With_Interleaved_Lines _describe_frame(void);        // determines frame format from configuration

      virtual void                 _config_digitizer(void);
      virtual void                 _config_daq(void);

      virtual void                 _generate_ao_waveforms(void); // fills ao_workspace with data for analog output
      virtual void                 _write_ao(void);
      
      virtual void                 _register_daq_event(void);
      virtual unsigned int         _wait_for_daq(DWORD timeout_ms); // returns 1 on succes, 0 otherwise
      
      //static  int32 CVICALLBACK    _daq_event_callback(TaskHandle taskHandle, int32 type, int32 nsamples, void *callbackData);

    protected: //Section: generators for ao waveforms
      static  void _compute_galvo_waveform__constant_zero        ( Scanner2D::Config *cfg, float64 *data, double N );
      static  void _compute_linear_scan_mirror_waveform__sawtooth( LinearScanMirror::Config *cfg, float64 *data, double N );
      static  void _compute_pockels_vertical_blanking_waveform   ( Pockels::Config *cfg, float64 *data, double N );

    private:
      void __common_setup();
    };

  } // end namespace device
}   // end namespace fetch
