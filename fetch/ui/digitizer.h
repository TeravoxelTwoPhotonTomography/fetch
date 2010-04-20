namespace fetch
{
  namespace ui
  {
    namespace digitizer
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
      class Menu
      { char       _name[512];
        Digitizer *_digitizer;
        HMENU      _menu,
                   _taskmenu;

        public:
          Menu(Digitizer *digitizer);

          LRESULT CALLBACK Handler ( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
          void             Append  ( HMENU hmenu );
          void             Insert  ( HMENU menu, UINT uPosition, UINT uFlags );

        private:

          // See common.h for macro.
          // All IDM_* are static const int.
          // The message are assigned in the constructor.
          static size_t _instance_id = 0;

          DECLARE_USER_MESSAGE_NON_STATIC( IDM_DIGITIZER);
          DECLARE_USER_MESSAGE_NON_STATIC( IDM_DIGITIZER_DETACH);
          DECLARE_USER_MESSAGE_NON_STATIC( IDM_DIGITIZER_ATTACH);
          DECLARE_USER_MESSAGE_NON_STATIC( IDM_DIGITIZER_LIST_DEVICES);
          DECLARE_USER_MESSAGE_NON_STATIC( IDM_DIGITIZER_TASK_STOP);
          DECLARE_USER_MESSAGE_NON_STATIC( IDM_DIGITIZER_TASK_RUN);
          DECLARE_USER_MESSAGE_NON_STATIC( IDM_DIGITIZER_TASK_0);
          DECLARE_USER_MESSAGE_NON_STATIC( IDM_DIGITIZER_TASK_1);

          struct __task_table_row
          { char     *menutext;
            const int messageid;
            Task      task;
          } _t_task_table, _t_task_table_row;
          static _t_task_table _task_table[] = {
            { "Fetch Forever  8-bit",   IDM_DIGITIZER_TASK_0, task::digitizer::FetchForever<i8>()  },
            { "Fetch Forever 16-bit",   IDM_DIGITIZER_TASK_1, task::digitizer::FetchForever<i16>() },
            { NULL, NULL, NULL },
          };
      
          HMENU _make_menu(void);
      };


    }
  }
}
