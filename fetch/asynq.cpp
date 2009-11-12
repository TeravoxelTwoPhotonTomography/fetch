#include "stdafx.h"
#include "asynq.h"

asynq*
Asynq_Alloc(size_t buffer_count, size_t buffer_size_bytes )
{ asynq *self = Guarded_Malloc( sizeof(asynq), "Asynq_Alloc" );
  
  self->q = DCQueue_Alloc( buffer_count, buffer_size_bytes );
  
  self->waiting_threads = 0;
  self->ref_count       = 1;

  // For spin count,
  //    set high bit to preallocate event used in EnterCriticalSection for W2K
  //    see: http://msdn.microsoft.com/en-us/library/ms683472(VS.85).aspx
  Guarded_Assert_WinErr( 
    InitializeCriticalSectionAndSpinCount( &self->lock, 
                                            0x80000400 ) 
  );
  Guarded_Assert_WinErr(
    self->notify = CreateEvent( NULL,   // default security attributes
                                FALSE,  // auto reset
                                FALSE,  // initial state is untriggered
                                NULL ); // unnamed
  );

  return self;
}

void 
_asynq_free( asynq *self )
{ return_if_fail(self);  
  DCQueue_Free( self->q );  
	DeleteCriticalSection( &self->lock );
	CloseHandle( self->notify );
	free(self);
}

asynq*
Asynq_Inc_Ref( asynq *self )
{ InterlockedIncrement( &self->ref_count );
  return self;
}

void
Asynq_Dec_Ref( asynq *self )
{ InterlockedIncrement( &self->ref_count );
  return self;
}