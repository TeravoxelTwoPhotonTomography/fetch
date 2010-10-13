/*
 * ResonantWrap.h
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

#pragma once

#include "workers.pb.h"
#include "WorkTask.h"

#define RESONANTWRAPAGENT_DEFAULT_TIMEOUT         INFINITE

namespace fetch
{

  namespace task
  {

    class ResonantWrap : public fetch::task::OneToOneWorkTask<Frame>
    {
      public:
        unsigned int work(IDevice *d, Frame *dst, Frame *src);
    };

  } // namespace task

  namespace worker {

    class ResonantWrapAgent:public WorkAgent<task::ResonantWrap,cfg::worker::ResonantWrap>
    {
      public:
        ResonantWrapAgent();
        ResonantWrapAgent(Config *config);

        void __common_setup();

        ~ResonantWrapAgent();


        void setTurn(float turn);
        int  setTurnNoWait(float turn);

        inline bool isInBounds(void);
        bool waitForOOBUpdate(DWORD timeout_ms); //returns true on out-of-bounds flag update.  Otherwise false (timeouts, abort, etc...)

      private:
        friend class task::ResonantWrap;
        void setIsInBounds(bool);

      private:
        HANDLE _notify_out_of_bounds_update;      //anybody waiting on this should reset it first        
        bool _is_in_bounds;
    };

  }  // namespace worker

}

