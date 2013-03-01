/*
 * Scanner3D.h
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
 /** \file
     Scanner3D implementation
 */

#pragma once
#include "scanner2d.h"
#include "ZPiezo.h"
#include "scanner3d.pb.h"

namespace fetch
{


  bool operator==(const cfg::device::Scanner3D& a, const cfg::device::Scanner3D& b);
  bool operator!=(const cfg::device::Scanner3D& a, const cfg::device::Scanner3D& b);

  namespace device
  {
  /**
  This class models a device capable of point scanned volume acquisition using
  a resonant mirror, linear scan mirror, and a closed-loop piezo-drive
  objective positioner.

  Acquisition using a resonant mirror requires synchronizing multiple devices.
  Here, I've broken the operation of the scanner into many parts.
  The hierarchy looks like this:

  \verbatim
  (IDevice)-----|--------------|
            Digitizer      NIDAQAgent----|-------|--------------|-----------|
                |                     Pockels  Shutter  LinearScanMirror  ZPiezo
                 \                      /       /              /           /
                  \____________________/_______/______________/           /
                            |                                            /
                         Scanner2D                                      /
                            | _________________________________________/
                            |/
                         Scanner3D
  \endverbatim

  \section Notes
   - on_attach/on_detach just call Scanner2D's implementations.  Redefining them here
     just makes the call explicit (could implicitly do it with inheritence order, I think).

   - writing enough data for a frame across three channels requires more than the
     available memory on the DAQ.  However, we can transfer data faster than we use it.
     So we need to set this up to write the waveform out in pieces.
     Anyway, there are two relevant DAQmx properties, I think:
     \verbatim
       DAQmx.Channel >> AO.DataXFerReqCond set    to  DAQmx_Val_OnBrdMemNotFull
       DAQmx.Write   >> RegenMode                 to  DAQmx_Val_DoNotAllowRegen
     \endverbatim
 */
    class Scanner3D:public IScanner, public IConfigurableDevice<cfg::device::Scanner3D>
    {
    public:
      Scanner2D _scanner2d;
      ZPiezo    _zpiezo;
    public:
      Scanner3D(Agent *agent);
      Scanner3D(Agent *Agent, Config *cfg);

      void __common_setup();

      virtual unsigned int on_attach();                                         ///< \returns 0 on success, 1 on failure
      virtual unsigned int on_detach();                                         ///< \returns 0 on success, 1 on failure

      unsigned int on_disarm();

      virtual void _set_config(Config IN *cfg);
      virtual void _set_config(const Config& cfg);

      virtual int  onConfigTask();
      virtual void onUpdate() {_scanner2d._digitizer.onUpdate(); generateAO();}
      virtual void generateAO();
              void generateAOConstZ();
              void generateAOConstZ(float z_um);
              void generateAORampZ();
              void generateAORampZ(float z_um);
      virtual int  writeAO();                                                    ///< \returns 0 on success, 1 on failure
      virtual int  writeLastAOSample();

      virtual Scanner2D* get2d() {return &_scanner2d;}

      ZPiezo* zpiezo() {return &_zpiezo;}
    };

  }
}
