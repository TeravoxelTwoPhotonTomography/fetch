/*
 * Author: Nathan Clack <clackn@janelia.hhmi.org>
 *   Date: Apr 20, 2010
 */
/*
 * Copyright 2010 Howard Hughes Medical Institute.
 * All rights reserved.
 * Use is subject to Janelia Farm Research Campus Software Copyright 1.1
 * license terms (http://license.janelia.org/license/jfrc_copyright_1_1.html).
 */
#pragma once
#include "stdafx.h"
#include "agent.h"

#ifndef CALLBACK
#define CALLBACK
#endif

namespace fetch
{
  namespace ui
  {
    // ====
    // Menu
    // ====
    //
    // Add's a menu interface for controlling the state of the Digitizer and
    // for running different tasks.
    //
    // One should be able to have multiple sets of these menu's, each for
    // a different digitizer.  In fact, it's almost generalizable to any
    // Agent...
    //
    // Interface
    // ---------
    // Insert      should be called during WM_CREATE for a parent window.
    //             adds a menu for the digitizer
    //
    // Append      Like Insert_Menu, but used to add to another menu
    //             (similar to the win32 Append_Menu)
    //             Doesn't seemed to be used anywhere...
    //
    // Handler     should be called during the parent's WndProc
    //             handles events from the menu
    class AgentStateMenu
    {
    public:
      AgentStateMenu(Agent *agent);

      LRESULT CALLBACK Handler (HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
      void              Append (HMENU hmenu);
      void              Insert (HMENU menu, UINT uPosition, UINT uFlags);

    protected:
      char   _name[512];
      Agent *_agent;
      HMENU  _menu,
             _taskmenu;

      // See common.h for macro.
      // All IDM_* are static const int.
      // The message are assigned in the constructor.
      static size_t _instance_id = 0;

      DECLARE_USER_MESSAGE_NON_STATIC( IDM_AGENT);
      DECLARE_USER_MESSAGE_NON_STATIC( IDM_AGENT_DETACH);
      DECLARE_USER_MESSAGE_NON_STATIC( IDM_AGENT_ATTACH);
      DECLARE_USER_MESSAGE_NON_STATIC( IDM_AGENT_LIST_DEVICES);
      DECLARE_USER_MESSAGE_NON_STATIC( IDM_AGENT_TASK_STOP);
      DECLARE_USER_MESSAGE_NON_STATIC( IDM_AGENT_TASK_RUN);

      struct __t_task_table_row
      {
        char *menutext;
        const int messageid;
        Task task;
      } _t_task_table, _t_task_table_row;

      static _t_task_table _task_table[] =
        {
          { NULL, NULL, NULL },
        };

      HMENU _make_menu(void);
    };

  }
}
