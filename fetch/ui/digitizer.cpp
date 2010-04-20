#include "stdafx.h"
#include "util/util-niscope.h"
#include "device/digitizer.h"
#include "task/fetch-forever.h"

//
// [x] resolve accesss to g_digitizer.
// [x] resolve accesss to gp_digitizer_device.
// [x] resolve access to gp_digitizer_tasks
// [x] translate device function calls
//
// [x] should add menu name to constructor
//



namespace fetch
{
  namespace ui
  {
    namespace digitizer
    {
      Menu::Menu(char *name, device::Digitizer *digitizer):
        _digitizer(digitizer),
        _menu(0),
        _taskmenu(0)
        {
          assert(strlen(name)<sizeof(_name));
          memset(_name,0,sizeof(_name));
          memcpy(_name,name,strlen(name));

          ++_instance_id;
          DEFINE_USER_MESSAGE_INT__NON_STATIC( IDM_DIGITIZER,              _instance_id);
          DEFINE_USER_MESSAGE_INT__NON_STATIC( IDM_DIGITIZER_DETACH,       _instance_id);
          DEFINE_USER_MESSAGE_INT__NON_STATIC( IDM_DIGITIZER_ATTACH,       _instance_id);
          DEFINE_USER_MESSAGE_INT__NON_STATIC( IDM_DIGITIZER_LIST_DEVICES, _instance_id);
          DEFINE_USER_MESSAGE_INT__NON_STATIC( IDM_DIGITIZER_TASK_STOP,    _instance_id);
          DEFINE_USER_MESSAGE_INT__NON_STATIC( IDM_DIGITIZER_TASK_RUN,     _instance_id);  
          DEFINE_USER_MESSAGE_INT__NON_STATIC( IDM_DIGITIZER_TASK_0,       _instance_id);  
          DEFINE_USER_MESSAGE_INT__NON_STATIC( IDM_DIGITIZER_TASK_1,       _instance_id);


        }

      HMENU 
        Menu::_make_menu(void)
        { _menu = CreatePopupMenu(),
          _taskmenu = CreatePopupMenu();

          // Loop over task table, adding menu entries for each row.
          { _t_task_table_row *ttr;
            for(ttr=_task_table; ttr->menutext; ++ttr )
              Guarded_Assert_WinErr(AppendMenu(_task_table,MF_STRING,ttr->messageid,ttr->menutext));
          }
                                              
          Guarded_Assert_WinErr( AppendMenu( _menu, MF_STRING, IDM_DIGITIZER_DETACH,  "&Detach"));
          Guarded_Assert_WinErr( AppendMenu( _menu, MF_STRING, IDM_DIGITIZER_ATTACH, "&Attach"));
          Guarded_Assert_WinErr( AppendMenu( _menu, MF_STRING | MF_POPUP,  (UINT_PTR) taskmenu, "&Tasks"));
          Guarded_Assert_WinErr( AppendMenu( _menu, MF_STRING, IDM_DIGITIZER_TASK_RUN,  "&Run" ));
          Guarded_Assert_WinErr( AppendMenu( _menu, MF_STRING, IDM_DIGITIZER_TASK_STOP, "&Stop" ));
          Guarded_Assert_WinErr( AppendMenu( _menu, MF_SEPARATOR, NULL, NULL ));
          Guarded_Assert_WinErr( AppendMenu( _menu, MF_STRING, IDM_DIGITIZER_LIST_DEVICES, "&List NI modular devices"));
        }

      void 
        Menu::Append( HMENU menu )
        { _make_menu();  
          Guarded_Assert_WinErr( AppendMenu( menu, MF_STRING | MF_POPUP, (UINT_PTR) _menu, _name)); 
        }

      // Inserts the digitizer menu before the specified menu item
      // uFlags should be either MF_BYCOMMAND or MF_BYPOSITION.
      void 
        Menu::Insert( HMENU menu, UINT uPosition, UINT uFlags )
        { _make_menu();  
          Guarded_Assert_WinErr( 
            InsertMenu( menu,
                        uPosition,
                        uFlags | MF_STRING | MF_POPUP,
                        (UINT_PTR) _submenu,
                        _name)); 
        }

        // This has the type of a WinProc
        // Returns:
        //    0  if the message was handled
        //    1  otherwise
        LRESULT CALLBACK
          Handler(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
          { int wmId, wmEvent;

            switch (message)
            {
            case WM_INITMENU:
            case WM_INITMENUPOPUP:
              { HMENU hmenu = (HMENU) wParam;
                if( hmenu == _menu )
                { Guarded_Assert_WinErr(-1!=
                    EnableMenuItem( hmenu, 1 /*Attach*/, MF_BYPOSITION
                                    | ( (_digitizer->vi==0)?MF_ENABLED:MF_GRAYED) ));
                  Guarded_Assert_WinErr(-1!=
                    EnableMenuItem( hmenu, 2 /*Tasks*/, MF_BYPOSITION
                                    | ( (_digitizer->vi==0)?MF_GRAYED:MF_ENABLED) ));
                  Guarded_Assert_WinErr(-1!=
                    CheckMenuItem( hmenu, 2 /*Tasks*/, MF_BYPOSITION
                                    | ( _digitizer->is_runnable()?MF_CHECKED:MF_UNCHECKED) ));
                  Guarded_Assert_WinErr(-1!=
                    EnableMenuItem( hmenu, 3 /*Run*/, MF_BYPOSITION
                                    | ( _digitizer->is_runnable()?MF_ENABLED:MF_GRAYED) ));
                  Guarded_Assert_WinErr(-1!=
                    CheckMenuItem( hmenu, 3 /*Run*/, MF_BYPOSITION
                                    | ( _digitizer->is_running()?MF_CHECKED:MF_UNCHECKED) ));
                  Guarded_Assert_WinErr(-1!=
                    EnableMenuItem( hmenu, 4 /*Stop*/, MF_BYPOSITION
                                    | ( _digitizer->is_running()?MF_ENABLED:MF_GRAYED) ));	                        
          //DONE? update for running and stop
                  Guarded_Assert(-1!=             // Detect state
                    CheckMenuRadioItem(_menu,     // Toggle radio button to reflect
                                      0,1,((_digitizer->vi==0)?0/*Off*/:1/*Hold*/), MF_BYPOSITION ) );
                } else if( hmenu == _taskmenu )
                { // - Identify if any tasks are running.
                  //   If there is one, check that one and gray them all out.
                  // - Assumes items in the tasks submenu correspond with the
                  //   items in the task table.
                  int i,n;
                  i = n = GetMenuItemCount(hmenu);
                  debug("Task Menu: found %d items\r\n",n);
                  if( _digitizer->is_runnable() || _digitizer->is_running() )
                  { 
                    _t_task_table_row *ttr;                                      // Search for armed task
                    for(i=0,ttr=_task_table; ttr->menutext; ++i,++ttr)
                      if( Task::eq(_digitizer_task,&ttr->task) )
                        break;

                    Guarded_Assert(-1!=
                      CheckMenuRadioItem(hmenu,0,n-1,i, MF_BYPOSITION ) );        // mark the readio button indicator
                  }	else {                                                        // ...or...
                    Guarded_Assert(-1!=                                           // clear the radio button indicator
                      CheckMenuRadioItem(hmenu,0,n-1,-1, MF_BYPOSITION ) );
                  }      
                  { int i = GetMenuItemCount(hmenu);                              // Disable/Enable all tasks if Running/Not running
                    while(i--)
                      Guarded_Assert_WinErr(-1!=
                        EnableMenuItem( hmenu, i, MF_BYPOSITION 
                                        | (_digitizer->is_running()?MF_GRAYED:MF_ENABLED)));
                  }
                }
                return 1; // return 1 so that init menu messages are still processed by parent
              }
              break;
            case WM_COMMAND:
              wmId    = LOWORD(wParam);
              wmEvent = HIWORD(wParam);
              // Parse the menu selections:
              if( wmId == IDM_DIGITIZER_DETACH )
              { debug( "IDM_DIGITIZER_DETACH\r\n" );
                _digitizer->detach_nonblocking();
                
              } else if( wmId == IDM_DIGITIZER_ATTACH )
              { debug("IDM_DIGITIZER_ATTACH\r\n");
                _digitizer->attach();

              } else if( wmId == IDM_DIGITIZER_LIST_DEVICES )
              { debug("IDM_DIGITIZER_LIST_DEVICES\r\n");
                niscope_debug_list_devices();

              } else if( wmId == IDM_DIGITIZER_TASK_RUN )
              { debug("IDM_DIGITIZER_RUN\r\n");      
                _digitizer->run_nonblocking();
                
              } else if( wmId == IDM_DIGITIZER_TASK_STOP )
              { debug("IDM_DIGITIZER_STOP\r\n");
                _digitizer->stop_nonblocking(DIGITIZER_DEFAULT_TIMEOUT);
                
              } else
              { 
                // Loop over task table, looking for correspondance with
                // message.  Task instances are static class variables, so they
                // should be valid for the lifetime of the application.
                _t_task_table_row *ttr;
                for(ttr=_task_table; ttr->menutext; ++ttr )
                  if( wmID == ttr->messageid )
                  { _digitizer->arm_nonblocking(&ttr->task,DIGITIZER_DEFAULT_TIMEOUT);
                    return 0;
                  }
                return 1; // didn't find any matches for the message
              }
              break;
            default:
              return 1;
            }
            return 0;
          }

    } // end namespace digitizer
  }   // end namespace ui
}     // end namespace fetch
