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

#include "WorkTask.h"

#define RESONANTWRAPAGENT_DEFAULT_TIMEOUT         INFINITE

namespace fetch
{

  namespace task
  {

    class ResonantWrap : public fetch::task::OneToOneWorkTask<Frame>
    {
      public:
        unsigned int work(Agent *agent, Frame *dst, Frame *src);
    };

  } // namespace task

  namespace worker {

    class ResonantWrapAgent:public WorkAgent<task::ResonantWrap,float>
    {
      public:
        ResonantWrapAgent();
        ~ResonantWrapAgent();

        inline bool Is_In_Bounds(void);
        int  Set_Turn(float turn,int timestamp); //returns a timestamp
        bool Set_Turn_NonBlocking(float turn);

        bool WaitForOOBUpdate(DWORD timeout_ms); //returns true on out-of-bounds flag update.  Otherwise false (timeouts, abort, etc...)

      private:
        friend class task::ResonantWrap;
        void SetIsInBounds(bool);

      private:
        HANDLE notify_out_of_bounds_update;      //anybody waiting on this should reset it first
        CRITICAL_SECTION local_state_lock;
        bool is_in_bounds;
    };

  }  // namespace worker

}

