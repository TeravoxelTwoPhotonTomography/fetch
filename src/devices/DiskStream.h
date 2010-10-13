/*
 * DiskStream.h
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
/*
 * Notes
 * -----
 * - The Agent::connect() function will take care of the input queues.
 *
 */
#pragma once

#include "common.h"
#include "agent.h"
#include "tasks/File.h"
#include "storage.pb.h"

#define DISKSTREAM_MAX_PATH         1024
#define DISKSTREAM_MAX_MODE         4
#define DISKSTREAM_DEFAULT_TIMEOUT  INFINITE

namespace fetch
{

  namespace device
  {
    
    class DiskStream: public IConfigurableDevice<cfg::device::DiskStream>
    {
      public:
        DiskStream(Agent *agent);
        DiskStream(Agent *agent, Config *config);
        DiskStream(Agent *agent, char *filename, char *mode);
        ~DiskStream();

        virtual unsigned int open  (char *filename, char *mode) = 0;
                unsigned int close (void);                           //synonymous with detach()
                unsigned int detach(void);                        

    public:        
      HANDLE  _hfile;

      protected:
        unsigned int attach(void);                                   // use open() instead
    };                                                               

    template<typename TReader,typename TWriter>
    class DiskStreamSpecialized : public DiskStream
    { public:
        TReader reader;
        TWriter writer;
        
        unsigned int open(char *filename, char *mode);
    };
    typedef DiskStreamSpecialized<task::file::ReadMessage,task::file::WriteMessage>      DiskStreamMessage;
    typedef DiskStreamSpecialized<task::file::ReadRaw    ,task::file::WriteRaw>          DiskStreamRaw;
    typedef DiskStreamSpecialized<task::file::ReadRaw    ,task::file::WriteMessageAsRaw> DiskStreamMessageAsRaw;

  }

}
