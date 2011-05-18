/*
 * util-wrap.cpp
 *
 *  Created on: May 17, 2010
 *      Author: Nathan Clack <clackn@janelia.hhmi.org>
 */
/*
 * Copyright 2010 Howard Hughes Medical Institute.
 * All rights reserved.
 * Use is subject to Janelia Farm Research Campus Software Copyright 1.1
 * license terms (http://license.janelia.org/license/jfrc_copyright_1_1.html).
 */
 
#include "common.h" 
#include "util-wrap.h"
 
namespace resonant_correction
{
  namespace wrap
  {
    bool
    get_dim(int w, int h, float turn, int *ow, int *oh)
    {
      int t = lroundf(turn);
      if (turn >= w)
        return 0;
      t = internal::min(t, w - t);
      *oh = 2 * h;
      *ow = t;
      return 1;
    }
  } // namespace wrap
} // namespace resonant_correction
