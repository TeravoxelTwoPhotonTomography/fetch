#pragma once

typedef unsigned int (*pf_microscope_hold_callback)(void);
typedef unsigned int (*pf_microscope_off_callback) (void);

TYPE_VECTOR_DECLARE( pf_microscope_hold_callback );
TYPE_VECTOR_DECLARE( pf_microscope_off_callback  );

void Microscope_Register_Application_Shutdown_Procs(void);

int Register_New_Microscope_Hold_Callback( pf_microscope_hold_callback callback );
int Register_New_Microscope_Off_Callback ( pf_microscope_off_callback  callback );

unsigned int Microscope_Off ( void );
unsigned int Microscope_Hold( void );

void Microscope_Application_Start(void);