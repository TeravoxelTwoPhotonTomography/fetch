/*
 * LinearScanMirror.cpp
 *
 *  Created on: Apr 20, 2010
 *      Author: clackn
 */

#include "LinearScanMirror.h"

namespace fetch
{

  namespace device
  {

    LinearScanMirror::LinearScanMirror()
                     : NIDAQAgent("LinearScanMirror"),
                       config((LinearScanMirror::Config)LINEAR_SCAN_MIRROR__DEFAULT_CONFIG)
    {}

  }

}
