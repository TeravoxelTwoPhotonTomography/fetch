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
#include "../stdafx.h"
#include "../common.h"
#include "../agent.h"
#include "../task.h"

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
      AgentStateMenu(char *name, Agent *agent);

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
      static size_t _instance_id;

      UINT IDM_AGENT;                        // These will be unique messages for each instance
      UINT IDM_AGENT_DETACH;
      UINT IDM_AGENT_ATTACH;
      UINT IDM_AGENT_LIST_DEVICES;
      UINT IDM_AGENT_TASK_STOP;
      UINT IDM_AGENT_TASK_RUN;

      struct _t_task_table
      {
        char     *menutext;
        UINT      messageid;
        Task     *task;
      };
      typedef _t_task_table _t_task_table_row;

      static _t_task_table _task_table[];

      HMENU _make_menu(void);
    };  
    
/* Children classes should initialize the task table according to the following pattern:

    AgentStateMenu::_t_task_table AgentStateMenu::_task_table[] = 
        {
          { NULL, NULL, NULL },
        }
 */; 

  }
}
