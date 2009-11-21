#pragma once
#include "stdafx.h"
#include "device.h"
#include "asynq.h"

typedef asynq* PASYNQ;

TYPE_VECTOR_DECLARE(PASYNQ);

typedef struct _device Device;
typedef unsigned int (*fp_device_task_cfg_proc)( Device *d );
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
 */

typedef struct _device_task
{ vector_PASYNQ           *in;         // Input  pipes
  vector_PASYNQ           *out;        // Output pipes
  fp_device_task_cfg_proc  cfg_proc;   // config procedure
  fp_device_task_run_proc  run_proc;   // acquisition procedure
  
  LPTHREAD_START_ROUTINE   main;       // thread procedure (wraps run_proc)
} DeviceTask;

DeviceTask *DeviceTask_Alloc( size_t num_inputs,
                              size_t input_queue_size, 
                              size_t input_buffer_size,
                              size_t num_outputs,
                              size_t output_queue_size,
                              size_t output_buffer_size,
                              fp_device_task_cfg_proc cfg,
                              fp_device_task_run_proc run );
void        DeviceTask_Free( DeviceTask *self );