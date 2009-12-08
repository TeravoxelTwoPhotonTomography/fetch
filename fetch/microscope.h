#pragma once

typedef unsigned int (*pf_microscope_attach_callback)(void);
typedef unsigned int (*pf_microscope_detach_callback) (void);

TYPE_VECTOR_DECLARE( pf_microscope_attach_callback );
TYPE_VECTOR_DECLARE( pf_microscope_detach_callback  );

void Microscope_Register_Application_Shutdown_Procs(void);

size_t Register_New_Microscope_Attach_Callback( pf_microscope_attach_callback callback );
size_t Register_New_Microscope_Detach_Callback ( pf_microscope_detach_callback  callback );

unsigned int Microscope_Detach ( void );
unsigned int Microscope_Attach( void );

void Microscope_Application_Start(void);