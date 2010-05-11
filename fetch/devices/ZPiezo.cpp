/*
 * ZPiezo.cpp
 *
 *  Created on: May 7, 2010
 *      Author: Nathan Clack <clackn@janelia.hhmi.org>
 */
 /*
 * Copyright 2010 Howard Hughes Medical Institute.
 * All rights reserved.
 * Use is subject to Janelia Farm Research Campus Software Copyright 1.1
 * license terms (http://license.janelia.org/license/jfrc_copyright_1_1.html).
 */
#include "StdAfx.h"
#include "ZPiezo.h"

namespace fetch
{ namespace device
  {
  
    ZPiezo::Config::Config()
           :um2v     ( DEFAULT_ZPIEZO_UM2V     ),
            um_max   ( DEFAULT_ZPIEZO_UM_MAX   ),
            um_min   ( DEFAULT_ZPIEZO_UM_MIN   ),
            um_step  ( DEFAULT_ZPIEZO_UM_STEP  ),
            v_lim_max( DEFAULT_ZPIEZO_V_MAX    ),
            v_lim_min( DEFAULT_ZPIEZO_V_MIN    ),
            v_offset ( DEFAULT_ZPIEZO_V_OFFSET )
    { Guarded_Assert( sizeof(DEFAULT_ZPIEZO_CHANNEL ) < sizeof(channel) );
      memcpy(channel, DEFAULT_ZPIEZO_CHANNEL, sizeof(DEFAULT_ZPIEZO_CHANNEL));
    }

    ZPiezo::ZPiezo(void)
           : NIDAQAgent("ZPiezo")
    {}
  
  }
}
