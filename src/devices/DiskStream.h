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
 * - This has got to be the worst possible way...ugh
 *
 */
#pragma once

#include "common.h"
#include "agent.h"
#include "tasks/File.h"
#include "file.pb.h"
#include "util/util-mylib.h"
#include <vector>

#define DISKSTREAM_MAX_PATH         1024
#define DISKSTREAM_MAX_MODE         4
#define DISKSTREAM_DEFAULT_TIMEOUT  INFINITE
#define DISKSTREAM_CURRENT_FREEBYTES 10000000000

namespace fetch
{

  bool operator==(const cfg::File& a, const cfg::File& b);
  bool operator!=(const cfg::File& a, const cfg::File& b);

  namespace device
  {

    class IDiskStream: public IConfigurableDevice<cfg::File>
    {
      public:
        IDiskStream(Agent *agent);
        IDiskStream(Agent *agent, Config *config);
        IDiskStream(Agent *agent, char *filename, char *mode);

        virtual unsigned int open(const std::string& filename, const std::string& mode);     // 0=success, 1=failure.  Attaches, arms, and runs
        inline  unsigned int close (void) {return _agent->detach();};                        //synonymous with detach()

        void flush(); // Wait till input queues are empty

        // Children still need to define
        //   on_detach()
        //   on_attach()
        // Children need to set the _reader and _writer pointers

    protected:
        Task *_reader;
        Task *_writer;

    };

    class TiffStream : public IDiskStream
    {
    public:
      TiffStream(Agent *agent);
      TiffStream(Agent *agent, Config *config);

      unsigned int on_detach();
    protected:
      unsigned int on_attach();

      unsigned int _attach_writer( char * filename );

      unsigned int _attach_reader( char * filename );

    public:
      task::file::TiffStreamReadTask  _read_task;
      task::file::TiffStreamWriteTask _write_task;
      mylib::Tiff_Reader *_tiff_reader;
      mylib::Tiff_Writer *_tiff_writer;
    };

    class TiffGroupStream : public IDiskStream
    {
      int nchan_;
    public:
      TiffGroupStream(Agent *agent);
      TiffGroupStream(Agent *agent, Config *config);

      void set_nchan(int nchan) {nchan_=nchan;}
      int nchan()               {return nchan_;}

      unsigned int on_detach();
    protected:
      unsigned int on_attach();

      unsigned int _attach_writer( char * filename );
      unsigned int _attach_reader( char * filename );

    public:
      task::file::TiffGroupStreamReadTask  _read_task;
      task::file::TiffGroupStreamWriteTask _write_task;
      std::vector<mylib::Tiff*> _readers;
      std::vector<mylib::Tiff*> _writers;
    };

    class HFILEDiskStreamBase : public IDiskStream
    {
      // Children still need to define
      //   on_detach()
      //   on_attach()
      // Children need to set the _reader and _writer pointers
    public:
      HFILEDiskStreamBase(Agent *agent);
      HFILEDiskStreamBase(Agent *agent, Config *config);
    public:
      HANDLE  _hfile;
    };

    template<typename TReader,typename TWriter>
    class HFILEDiskStream : public HFILEDiskStreamBase
    {
    public:
      HFILEDiskStream(Agent *agent);
      HFILEDiskStream(Agent *agent, Config *config);

      unsigned int on_detach(void);

    protected:
      unsigned int on_attach(void);                       // use open() instead

    private:
      TReader _inst_reader;
      TWriter _inst_writer;
    };


    typedef HFILEDiskStream<task::file::ReadMessage,task::file::WriteMessage>      DiskStreamMessage;
    typedef HFILEDiskStream<task::file::ReadRaw    ,task::file::WriteRaw>          DiskStreamRaw;
    typedef HFILEDiskStream<task::file::ReadRaw    ,task::file::WriteMessageAsRaw> DiskStreamMessageAsRaw;

  }

}
