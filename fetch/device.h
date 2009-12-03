#pragma once

#include "stdafx.h"
#include "device-task.h"

typedef struct _device_task DeviceTask;

typedef struct _device
{ HANDLE             thread;             // thread used to run the task process
  HANDLE             notify_available;   // event used to signal threads waiting to add a task (manual reset)
  HANDLE             notify_stop;        // used to signal termination to the task (auto reset)
  CRITICAL_SECTION   lock;               // mutex to synchronize access to a device
  u32                num_waiting;        // number of threads waiting to add a task  
  u32                is_available;       // flag identifies when the device is accepting tasks
  u32                is_running;         // flag identifies when the device is running
  DeviceTask        *task;               // pointer to current task...not owned by this object
  void              *context;            // Resource handle/description
} Device;

Device *Device_Alloc(void);
void    Device_Free(Device *self);

// State-change functions
// ----------------------
unsigned int Device_Arm    ( Device *self, DeviceTask *task, DWORD timeout_ms );
unsigned int Device_Disarm ( Device *self, DWORD timeout_ms );
unsigned int Device_Run    ( Device *self );
unsigned int Device_Stop   ( Device *self, DWORD timeout_ms );

// Nonblocking state-change functions
// ----------------------------------
// These push the normal stat-change functions to the Windows thread queue
// and return immediately.  Each returns the result of the QueueUserWorkItem call.
BOOL Device_Arm_Nonblocking    ( Device *self, DeviceTask *task, DWORD timeout_ms );
BOOL Device_Disarm_Nonblocking ( Device *self, DWORD timeout_ms );
BOOL Device_Run_Nonblocking    ( Device *self );
BOOL Device_Stop_Nonblocking   ( Device *self, DWORD timeout_ms );

// State Query functions
// ---------------------
unsigned int Device_Is_Available  ( Device *self );
unsigned int Device_Is_Armed      ( Device *self );
unsigned int Device_Is_Running    ( Device *self );

// Utilities
// ---------
inline void Device_Lock   ( Device *self );
inline void Device_Unlock ( Device *self );
