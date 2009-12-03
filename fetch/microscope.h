#pragma once

typedef unsigned int (*pf_microscope_attach_callback)(void);
typedef unsigned int (*pf_microscope_off_callback) (void);

TYPE_VECTOR_DECLARE( pf_microscope_attach_callback );
TYPE_VECTOR_DECLARE( pf_microscope_off_callback  );

void Microscope_Register_Application_Shutdown_Procs(void);

size_t Register_New_Microscope_Attach_Callback( pf_microscope_attach_callback callback );
size_t Register_New_Microscope_Off_Callback ( pf_microscope_off_callback  callback );

unsigned int Microscope_Off ( void );
unsigned int Microscope_Attach( void );

void Microscope_Application_Start(void);