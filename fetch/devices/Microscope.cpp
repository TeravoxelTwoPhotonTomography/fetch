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

    Microscope::Config::Config()
    : file_iter_id(0)
    {    
      Guarded_Assert(sizeof(MICROSCOPE_DEFAULT_DEST_PATH)<sizeof(path));
      Guarded_Assert(sizeof(MICROSCOPE_DEFAULT_FILE_NAME_TEMPLATE)<sizeof(filename_template));
      memcpy(path,MICROSCOPE_DEFAULT_DEST_PATH,sizeof(MICROSCOPE_DEFAULT_DEST_PATH));
      memcpy(filename_template,MICROSCOPE_DEFAULT_FILE_NAME_TEMPLATE,sizeof(MICROSCOPE_DEFAULT_FILE_NAME_TEMPLATE));
    }

    Microscope::Microscope()
    : scanner(),
      frame_averager(4),
      pixel_averager(4),
      cast_to_i16(),
      trash(),
      disk()
    {}  
    
    Microscope::~Microscope(void)
    { if( this->detach()>0 )
        warning("Could not detach microscope (address 0x%p).\r\n",this);
    } 

    unsigned int
    Microscope::attach(void)
    { int sts = 0; // 0 success, 1 failure
      sts |= scanner.attach();
      sts |= disk.open("default.stream","w");
      this->lock();
      this->set_available();
      this->unlock();
      return sts;  
    }
    
    unsigned int
    Microscope::detach(void)
    { int sts = 0; // 0 success, 1 failure
      if(!disarm(MICROSCOPE_DEFAULT_TIMEOUT))
         warning("Microscope::detach(): Could not cleanly disarm microscope.\n");
      sts |= scanner.detach();
      sts |= frame_averager.detach();
      sts |= pixel_averager.detach();
      sts |= trash.detach();
      sts |= disk.detach();
      this->_is_available = 0;
      return sts;  
    }
    
    unsigned int Microscope::disarm(DWORD timeout_ms)
    { unsigned int sts = 1; // success      
      sts &= scanner.disarm(timeout_ms);
      sts &= frame_averager.disarm(timeout_ms);
      sts &= pixel_averager.disarm(timeout_ms);
      sts &= trash.disarm(timeout_ms);
      sts &= disk.disarm(timeout_ms);
      
      sts &= Agent::disarm(timeout_ms);  
      return sts;
    }
    
    void Microscope::next_filename(char *dest)
    { static char fn[MAX_PATH];
      char *c;
      size_t sz;
      lock();
      { memset(fn,0,sizeof(fn));
        c = fn;
        sz = strlen(this->config.path);
        memcpy(c,this->config.path,sz);
        c+=sz;
        sz = strlen(this->config.filename_template);
        memcpy(c,this->config.filename_template,sz);
        sprintf(dest,fn,this->config.file_iter_id++);
      }
      unlock();
    }
    
  } // end namespace device  
} // end namespace fetch
