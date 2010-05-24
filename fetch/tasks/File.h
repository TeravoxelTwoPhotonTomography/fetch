/*
 * File.h
 *
 *  Created on: Apr 23, 2010
 *      Author: Nathan Clack <clackn@janelia.hhmi.org>
 */
/*
 * Copyright 2010 Howard Hughes Medical Institute.
 * All rights reserved.
 * Use is subject to Janelia Farm Research Campus Software Copyright 1.1
 * license terms (http://license.janelia.org/license/jfrc_copyright_1_1.html).
 */
#pragma once

#include "../devices/DiskStream.h"
#include "../task.h"



namespace fetch
{ namespace device { class DiskStream; }

  namespace task
  {
    namespace file {

      //typedef UpcastTask<device::DiskStream> DiskStreamTask;
      typedef Task DiskStreamTask;

      /*
      WriteRaw
        Writes all the bytes popped from the queue to disk.
        The buffer size of the input queue determines the 
        size of the chunk written to disk in a single call.
        
      ReadRaw
        Reads raw data to disk and pushes it onto an output
        queue.  The amount of data requested on each read
        is determined by the output queue size.
      */
      class WriteRaw : public DiskStreamTask
      { public:
          unsigned int config (Agent *d);
          unsigned int run    (Agent *d);
                    
          unsigned int config(device::DiskStream *agent);
          unsigned int run(device::DiskStream *agent);
      };

      class ReadRaw : public DiskStreamTask
      { public:
          unsigned int config (Agent *d);
          unsigned int run    (Agent *d);
            
          unsigned int config(device::DiskStream *agent);
          unsigned int run(device::DiskStream *agent);
      };

      class WriteMessage : public DiskStreamTask
      { public:  
          unsigned int config (Agent *d);
          unsigned int run    (Agent *d);
            
          unsigned int config(device::DiskStream *agent);
          unsigned int run(device::DiskStream *agent);
      };

      class ReadMessage : public DiskStreamTask
      { public:
          unsigned int config (Agent *d);
          unsigned int run    (Agent *d);
            
          unsigned int config(device::DiskStream *agent);
          unsigned int run(device::DiskStream *agent);
      }; 
      
      // Extracts data from messages and writes just the data to disk.
      class WriteMessageAsRaw : public DiskStreamTask
      { public:  
          unsigned int config (Agent *d);
          unsigned int run    (Agent *d);
            
          unsigned int config(device::DiskStream *agent);
          unsigned int run(device::DiskStream *agent);
      };

    }  // namespace disk

  }

}
