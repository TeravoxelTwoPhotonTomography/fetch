#include "StdAfx.h"
#include "Scanner3D.h"
#include "Scanner2D.h"


namespace fetch
{ namespace device
  {
    Scanner3D::Scanner3D(void)
    {
    }

    Scanner3D::~Scanner3D(void)
    { if (this->detach()>0)
          warning("Could not cleanly detach Scanner3D. (addr = 0x%p)\r\n",this);
    }
    
    unsigned int
    Scanner3D::
    attach(void) 
    { return this->Scanner2D::attach(); // Returns 0 on success, 1 otherwise
    }
    
    unsigned int
    Scanner3D::
    detach(void)
    { return this->Scanner2D::detach(); // Returns 0 on success, 1 otherwise
    }
    
  }
}
