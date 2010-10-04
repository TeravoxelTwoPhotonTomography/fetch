#include "stdafx.h"

namespace mylib 
{

LPCRITICAL_SECTION _gp_cs = NULL;

static LPCRITICAL_SECTION _get_cs(void)
{ static CRITICAL_SECTION gcs;
  if(!_gp_cs)
  { InitializeCriticalSectionAndSpinCount( &gcs, 0x80000400 );
    _gp_cs = &gcs;
  }
  
  return _gp_cs;
}

static void _cleanup_mylib_critical_section(void)
{ if(_gp_cs)
    DeleteCriticalSection( _gp_cs );
  _gp_cs = NULL;
}

void lock(void)
{ EnterCriticalSection( _get_cs() );
}

void unlock(void)
{ LeaveCriticalSection( _get_cs() );
}

} //end namespace mylib
