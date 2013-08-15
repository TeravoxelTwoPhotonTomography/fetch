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
 *               |                     NIDAQPockels  Shutter  LinearScanMirror
 *                \                      /       /              /
 *                 \____________________/_______/______________/
 *                           |
 *                        Scanner2D
 *
 * The NIDAQAgent and Digitizer classes both provide the on_attach/on_detach
 * functions required of a class implementing the Agent interface.
 * The NIDAQAgent class is _not_ virtually inherited so each of the Pockels,
 * Shutter, and LinearScanMirror classes bring in a hardware context handle:
 * <daqtask>.  The handles all have the same name, so to unambiguously access
 * the <daqtask> member, a down cast is required.
 *
 * It's possible to get independent task handles for the pockels cell, the
 * shutter and the linear scan mirror by calling the on_attach() function for each.
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
 * form, Shutter::on_attach() _must_ be used in order to use Shutter::Open() and
 * Shutter::Closed().
 *
 * Clear as Mud.
 */
#pragma once
#include "digitizer.h"
#include "pockels.h"
#include "shutter.h"
#include "LinearScanMirror.h"
#include "frame.h"
#include "scanner2d.pb.h"
#include "object.h"
#include "daq.h"
#include <map>
#include <string>

#define SCANNER2D_DEFAULT_TIMEOUT               INFINITE // ms

namespace fetch
{

  bool operator==(const cfg::device::Scanner2D& a, const cfg::device::Scanner2D& b);
  bool operator!=(const cfg::device::Scanner2D& a, const cfg::device::Scanner2D& b);


  namespace device
  {
    class Scanner2D;

    class IScanner
    {
    public:
      IScanner() : _ao_workspace(NULL) {}

      virtual int  onConfigTask()   = 0;  ///< called in a 2d scanning task's config function
      virtual void generateAO() = 0;
      virtual int  writeAO()    = 0;      ///< \returns 0 on success, 1 on failure
      virtual int  writeLastAOSample()=0;

      virtual Scanner2D* get2d() = 0;

    protected:
      vector_f64 *_ao_workspace;
    };

    class Scanner2D : public IScanner, public IConfigurableDevice<cfg::device::Scanner2D>
    {
    public:
      Digitizer            _digitizer;
      DAQ                  _daq;
      Shutter              _shutter;
      LinearScanMirror     _LSM;
      Pockels              _pockels1,
                           _pockels2;

      std::map<cfg::device::Pockels::LaserLineIdentifier,Pockels*>    _pockels_map;
      std::map<std::string,cfg::device::Pockels::LaserLineIdentifier> _laser_line_map;

    public:
      Scanner2D(Agent *agent);
      Scanner2D(Agent *agent, Config *cfg);

      virtual unsigned int on_attach(); ///< \returns 0 on success, 1 on failure
      virtual unsigned int on_detach(); ///< \returns 0 on success, 1 on failure
      virtual unsigned int on_disarm(); ///< \returns 0 on success, 1 on failure

      virtual void _set_config(Config IN *cfg);
      virtual void _set_config(const Config& cfg);

      virtual int  onConfigTask();
      virtual void onUpdate() {_digitizer.onUpdate(); generateAO();}
      virtual void generateAO();
      virtual int  writeAO();            ///< \returns 0 on success, 1 on failure
      virtual int  writeLastAOSample();

      virtual Scanner2D* get2d() {return this;}

      Digitizer* digitizer() { return &_digitizer; }
      Chan *getVideoChannel() { return _out->contents[0]; }
      Pockels* pockels(const std::string& name);
      Pockels* pockels(unsigned idx);
      Pockels* pockels(cfg::device::Pockels::LaserLineIdentifier id);

    private:
      void __common_setup();
    };

  } // end namespace device
}   // end namespace fetch
