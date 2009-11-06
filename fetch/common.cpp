#include "stdafx.h"
#include "common.h"

#include <strsafe.h>

#include <stdlib.h>
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// --------
// Shutdown
// --------
//
// Shutdown callback functions
//   - take no arguments
//   - do not exit/abort under any circumstances
//   - return 0 only on success
//   - are called in reverse order of when they were
//     added.
//
// After all callbacks return, the system is assumed
// to be in a state where exit/abort can safely be
// called.
//

TYPE_VECTOR_DECLARE( pf_shutdown_callback );
TYPE_VECTOR_DEFINE ( pf_shutdown_callback );

vector_pf_shutdown_callback g_shutdown_callbacks = VECTOR_EMPTY;

int Register_New_Shutdown_Callback( pf_shutdown_callback callback )
{ int idx = g_shutdown_callbacks.count++;
  vector_pf_shutdown_callback_request( &g_shutdown_callbacks, idx );
  g_shutdown_callbacks.contents[idx] = callback;
  return idx;
}

unsigned int Shutdown_Soft(void)
{ size_t cnt = g_shutdown_callbacks.count;
  unsigned err = 0;
  pf_shutdown_callback *beg = g_shutdown_callbacks.contents,
                       *cur = beg+cnt;

  static int lock = 0;// Avoid recursion
  assert( lock == 0 ); 
  lock = 1;

  while( cur-- > beg )
    if(cur)
      err |= (*cur)();
  lock = 0;

  vector_pf_shutdown_callback_free_contents( &g_shutdown_callbacks );
  return err;
}

void Shutdown_Hard(unsigned int err)
{ exit( err | Shutdown_Soft() );
}

// ------------------------------------
// Error, Debug, and Warning reporting
// ------------------------------------

TYPE_VECTOR_DECLARE(pf_reporting_callback);
TYPE_VECTOR_DEFINE(pf_reporting_callback);

vector_pf_reporting_callback g_error_report_callbacks    = VECTOR_EMPTY;
vector_pf_reporting_callback g_warning_report_callbacks  = VECTOR_EMPTY;
vector_pf_reporting_callback g_debug_report_callbacks    = VECTOR_EMPTY;

int _register_new_reporting_callback( vector_pf_reporting_callback *vec,
                                      pf_reporting_callback callback )
{ size_t idx = vec->count++;
  vector_pf_reporting_callback_request( vec, idx );
  vec->contents[idx] = callback;
  return idx;
}

int Register_New_Error_Reporting_Callback( pf_reporting_callback callback )
{ return _register_new_reporting_callback( &g_error_report_callbacks, callback );
}

int Register_New_Warning_Reporting_Callback( pf_reporting_callback callback )
{ return _register_new_reporting_callback( &g_warning_report_callbacks, callback );
}

int Register_New_Debug_Reporting_Callback( pf_reporting_callback callback )
{ return _register_new_reporting_callback( &g_debug_report_callbacks, callback );
}

void _reporting_callbacks_remove_all( vector_pf_reporting_callback *vec )
{ vec->count = 0; 
}

void Error_Reporting_Remove_All_Callbacks(void)
{ _reporting_callbacks_remove_all( &g_error_report_callbacks );
}

void Debug_Reporting_Remove_All_Callbacks(void)
{ _reporting_callbacks_remove_all( &g_debug_report_callbacks );
}

void Warning_Reporting_Remove_All_Callbacks(void)
{ _reporting_callbacks_remove_all( &g_warning_report_callbacks );
}

//
// Log to debugger console
// -----------------------
//

void _reporting_log_to_console_callback( const char *msg )
{ OutputDebugString((TCHAR*)msg);
}

void Reporting_Setup_Log_To_Debugger_Console(void)
{ Register_New_Error_Reporting_Callback( &_reporting_log_to_console_callback );
  Register_New_Warning_Reporting_Callback( &_reporting_log_to_console_callback );
  Register_New_Debug_Reporting_Callback( &_reporting_log_to_console_callback );
}

//
// Log to file
// -----------
// A simple reporting setup
//

FILE *_g_reporting_log_file = NULL;

void _reporting_log_to_file_callback( const char *msg )
{ if( _g_reporting_log_file )
    fprintf( _g_reporting_log_file, msg );
}

unsigned int _reporting_log_to_file_shutdown_callback( void )
{ 
  if( _g_reporting_log_file ) 
  { int code = fclose(_g_reporting_log_file );
    _g_reporting_log_file = 0;
    return code;
  }
  
  return 0;
}

void Reporting_Setup_Log_To_File( FILE *file )
{ _g_reporting_log_file = file;
  if(file)
  { Register_New_Error_Reporting_Callback  ( &_reporting_log_to_file_callback );
    Register_New_Warning_Reporting_Callback( &_reporting_log_to_file_callback );
    Register_New_Debug_Reporting_Callback  ( &_reporting_log_to_file_callback );
    Register_New_Shutdown_Callback( &_reporting_log_to_file_shutdown_callback );
  }
}

//
// Main reporting functions
// ------------------------
//



void ReportLastWindowsError(void) 
{ 
    // Retrieve the system error message for the last-error code

    LPVOID lpMsgBuf;
    LPVOID lpDisplayBuf;
    DWORD dw = GetLastError(); 

    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | 
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        dw,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR) &lpMsgBuf,
        0, NULL );

    // Display the error message and exit the process

    lpDisplayBuf = (LPVOID)LocalAlloc(LMEM_ZEROINIT, 
        (lstrlen((LPCTSTR)lpMsgBuf) + 40) * sizeof(TCHAR)); 
    StringCchPrintf((LPTSTR)lpDisplayBuf, 
        LocalSize(lpDisplayBuf) / sizeof(TCHAR),
        TEXT("Failed with error %d: %s"), 
        dw, lpMsgBuf); 
    
    // spam formated string to listeners
    { static int lock = 0;
      assert( lock == 0 ); // One of the logging functions generated a log message
      lock = 1;
      { pf_reporting_callback *beg = g_error_report_callbacks.contents;
        size_t cnt                 = g_error_report_callbacks.count;
        pf_reporting_callback *cur = beg + cnt;
        while( cur-- > beg )
          if(cur)
            (*cur)((char*)lpDisplayBuf);
      }
      lock = 0;
    }

    LocalFree(lpMsgBuf);
    LocalFree(lpDisplayBuf);
}

void error(const char* fmt, ...)
{ static char prefix[] = "*** ERROR: ";
  size_t len;
  va_list argList;
  static vector_char vbuf = VECTOR_EMPTY;
  
  // render formated string
  va_start( argList, fmt );
    len = sizeof(prefix) + _vscprintf(fmt,argList) + 1;
    vector_char_request( &vbuf, len );
    memset  ( vbuf.contents, 0, len );
#pragma warning( push )
#pragma warning( disable:4996 )
#pragma warning( disable:4995 )
    sprintf ( vbuf.contents, prefix );
    vsprintf( vbuf.contents+sizeof(prefix)-1, fmt, argList);
#pragma warning( pop )
  va_end( argList );

  // spam formated string to listeners
  { static int lock = 0;
    assert( lock == 0 ); // One of the logging functions generated a log message
    lock = 1;
    { pf_reporting_callback *beg = g_error_report_callbacks.contents;
      size_t cnt                 = g_error_report_callbacks.count;
      char *buffer = vbuf.contents;
      pf_reporting_callback *cur = beg + cnt;
      while( cur-- > beg )
        if(cur)
          (*cur)(buffer);
    }
    lock = 0;
  }

  // cleanup and shutdown
  Shutdown_Hard(1);
}

void warning(const char* fmt, ...)
{ static char prefix[] = "--- WARNING: ";
  size_t len;
  va_list argList;
  static vector_char vbuf = VECTOR_EMPTY;

  static int lock = 0;
  assert( lock == 0 ); // One of the logging functions generated a log message
  lock = 1;

  // render formated string
  va_start( argList, fmt );
    len = sizeof(prefix) + _vscprintf(fmt,argList) + 1;
    vector_char_request( &vbuf, len );
    memset  ( vbuf.contents, 0, len );
#pragma warning( push )
#pragma warning( disable:4996 )
#pragma warning( disable:4995 )
    sprintf ( vbuf.contents, prefix );
    vsprintf( vbuf.contents+sizeof(prefix)-1, fmt, argList);
#pragma warning( pop )
  va_end( argList );

  // spam formated string to listeners
  { pf_reporting_callback *beg = g_warning_report_callbacks.contents;
    size_t cnt                 = g_warning_report_callbacks.count;
    char *buffer = vbuf.contents;
    pf_reporting_callback *cur = beg + cnt;
    while( cur-- > beg )
      if(cur)
        (*cur)(buffer);
  }
  lock = 0;
}

void debug(const char* fmt, ...)
{ size_t len;
  va_list argList;
  static vector_char vbuf = VECTOR_EMPTY;

  static int lock = 0;
  assert( lock == 0 ); // One of the logging functions generated a log message
  lock = 1;

  // render formated string
  va_start( argList, fmt );
    len = _vscprintf(fmt,argList) + 1;
    vector_char_request( &vbuf, len );
    memset  ( vbuf.contents, 0, len );
#pragma warning( push )
#pragma warning( disable:4996 )
#pragma warning( disable:4995 )
    vsprintf( vbuf.contents, fmt, argList);
#pragma warning( pop )
  va_end( argList );

  // spam formated string to listeners
  { pf_reporting_callback *beg = g_debug_report_callbacks.contents;
    size_t cnt                 = g_debug_report_callbacks.count;
    char *buffer = vbuf.contents;
    pf_reporting_callback *cur = beg + cnt;
    while( cur-- > beg )
      if(cur)
        (*cur)(buffer);
  }
  lock = 0;
}

//
// Memory
//

void *Guarded_Malloc( size_t nelem, const char *msg )
{ void *item = malloc( nelem );
  if( !item )
    error("Could not allocate memory.\n%s\n",msg);
  return item;
}

void *Guarded_Calloc( size_t nelem, size_t bytes_per_elem, const char *msg )
{ void *item = calloc( nelem, bytes_per_elem );
  if( !item )
    error("Could not allocate memory.\n%s\n",msg);
  return item;
}

void Guarded_Realloc( void **item, size_t nelem, const char *msg )
{ void *it = *item;
  assert(item);
  if( !it )
    it = malloc( nelem );
  else
    it = realloc( it, nelem );
  if( !it )
    error("Could not reallocate memory.\n%s\n",msg);
  *item = it;
}

void RequestStorage( void** array, size_t *nelem, size_t request, size_t bytes_per_elem, const char *msg )
{ size_t n = request+1;
  if( n <= *nelem ) return;
  *nelem = (size_t) (1.25 * n + 64 );
#ifdef DEBUG_REQUEST_STORAGE
  printf("REQUEST %7d bytes (%7d items) above current %7d bytes by %s\n",request * bytes_per_elem, request, *nelem * bytes_per_elem, msg);
#endif
  Guarded_Realloc( array, *nelem * bytes_per_elem, "Resize" );
}


//
// Container types
//
TYPE_VECTOR_DEFINE(char)
TYPE_VECTOR_DEFINE(u8)
TYPE_VECTOR_DEFINE(u32)
TYPE_VECTOR_DEFINE(i8)
TYPE_VECTOR_DEFINE(i32)
TYPE_VECTOR_DEFINE(f32)
TYPE_VECTOR_DEFINE(f64)
