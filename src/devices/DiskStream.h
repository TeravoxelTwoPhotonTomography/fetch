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
#include "file.pb.h"

#define DISKSTREAM_MAX_PATH         1024
#define DISKSTREAM_MAX_MODE         4
#define DISKSTREAM_DEFAULT_TIMEOUT  INFINITE

namespace fetch
{

  namespace device
  {
    
    class DiskStreamBase: public IConfigurableDevice<cfg::File>
    {
      public:
        DiskStreamBase(Agent *agent);
        DiskStreamBase(Agent *agent, Config *config);
        DiskStreamBase(Agent *agent, char *filename, char *mode);
        ~DiskStreamBase();

        virtual unsigned int open(const std::string& filename, const std::string& mode) = 0;
                unsigned int close (void);                           //synonymous with detach()
                unsigned int detach(void);                        

    public:        
      HANDLE  _hfile;

      protected:
        unsigned int attach(void);                                   // use open() instead
    };                                                               

    template<typename TReader,typename TWriter>
    class DiskStream : public DiskStreamBase
    { public:
        TReader reader;
        TWriter writer;
        
        DiskStream(Agent *agent) : DiskStreamBase(agent) {}
        DiskStream(Agent *agent, Config *config) : DiskStreamBase(agent,config) {}

        unsigned int open(const std::string& filename, const std::string& mode);
    };
    typedef DiskStream<task::file::ReadMessage,task::file::WriteMessage>      DiskStreamMessage;
    typedef DiskStream<task::file::ReadRaw    ,task::file::WriteRaw>          DiskStreamRaw;
    typedef DiskStream<task::file::ReadRaw    ,task::file::WriteMessageAsRaw> DiskStreamMessageAsRaw;

  }

}
