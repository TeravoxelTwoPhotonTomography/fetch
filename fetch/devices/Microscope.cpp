/*
 * Microscope.cpp
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
 
 #include "stdafx.h"
 #include "Microscope.h"
 
 namespace fetch
{ namespace device {
    Microscope::Microscope()
    : scanner(),
      frame_averager(4),
      pixel_averager(4),
      trash(),
      disk()
    {}
    
    Microscope::~Microscope() 
    {}

    unsigned int
    Microscope::attach(void)
    { int sts = 0; // 0 success, 1 failure
      sts |= scanner.attach();
      sts |= disk.open("default.stream","w");
      return sts;  
    }
    
    unsigned int
    Microscope::detach(void)
    { int sts = 0; // 0 success, 1 failure
      if(disarm(MICROSCOPE_DEFAULT_TIMEOUT))
         warning("Microscope::detach(): Could not cleanly disarm scanner.\n");
      sts |= scanner.detach();
      sts |= disk.detach();
      return sts;  
    }
    
    unsigned int Microscope::disarm(DWORD timeout_ms)
    { unsigned int sts = 0; // success      
      sts |= scanner.disarm(timeout_ms);
      
      sts |= Agent::disarm(timeout_ms);  
      return sts;
    }
    
  } // end namespace device  
} // end namespace fetch
