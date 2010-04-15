#pragma once
#include "stdafx.h"
#include "device.h"
#include "asynq.h"

typedef asynq* PASYNQ;

TYPE_VECTOR_DECLARE(PASYNQ);

typedef struct _device Device;

// Callback types:
//    return 1 on sucess, 0 otherwise
typedef unsigned int (*fp_device_task_cfg_proc)( Device *d, 
                                                 vector_PASYNQ *in, 
                                                 vector_PASYNQ *out );
typedef unsigned int (*fp_device_task_run_proc)( Device *d, 
                                                 vector_PASYNQ *in, 
                                                 vector_PASYNQ *out );

/*
 * DEVICE TASK
 * -----------
 *
 * This encapsulates a task one might want to run on a device.
 * This will get registered with a device and run on the control
 * thread allocated to that device after performing the 
 * configuration described by the "cfg_proc."
 *
 * The in and out pipes provide routes for asynchronous input 
 * and output to the "run_proc."
 *
 * A given "cfg_proc" and "run_proc" may be run on multiple/different
 * Device instances during an application.
 */

typedef struct _device_task
{ vector_PASYNQ           *in;         // Input  pipes
  vector_PASYNQ           *out;        // Output pipes
  fp_device_task_cfg_proc  cfg_proc;   // config procedure
  fp_device_task_run_proc  run_proc;   // acquisition procedure
  
  LPTHREAD_START_ROUTINE   main;       // thread procedure (wraps run_proc)
} DeviceTask;

DeviceTask *DeviceTask_Alloc( fp_device_task_cfg_proc  cfg,        // Configuration procedure (run when Device is armed)
                              fp_device_task_run_proc  run);       // Run procedure (run when Device is run)
void        DeviceTask_Free( DeviceTask *self );
void        DeviceTask_Free_Outputs( DeviceTask *self );
void        DeviceTask_Free_Outputs( DeviceTask *self );

void        DeviceTask_Alloc_Inputs(  DeviceTask* self,
                                          size_t num_inputs,            // Input:  # of pipes        
                                          size_t *input_queue_size,     //         # of buffers/pipe 
                                          size_t *input_buffer_size);   //         buffer size       
void        DeviceTask_Alloc_Outputs( DeviceTask* self,              
                                          size_t num_outputs,           // Output: # of pipes       
                                          size_t *output_queue_size,    //         # of buffers/pipe
                                          size_t *output_buffer_size);  //         buffer size
                                          
void        DeviceTask_Connect( DeviceTask *source,      size_t source_channel,
                                DeviceTask *destination, size_t destination_channel);
