#pragma once
#include "stdafx.h"
#include "device.h"
#include "device-task-worker.h"

//
// Intended to act as "softare devices" that transforms data.
//

typedef struct _worker
  { u32   index;                                  // index into the instance array
    char *alias;                                  // points to alias stored in indexed item
  } Worker;

#define WORKER_EMPTY {-1,NULL}

#define WORKER_ALIAS_LENGTH 256
typedef struct _worker_index_item
  { char         alias[DISK_STREAM_ALIAS_LENGTH];
    Worker       worker;
    Device      *device;
  } Worker_Index_Item;

#define WORKER_INDEX_ITEM_EMPTY = {"\0",WORKER_EMPTY,NULL};

TYPE_VECTOR_DECLARE(Worker_Index_Item);

//
// Module interface
//

Worker*      Worker_Init                     (const char* alias);     // Allocs a Worker device with the alias.  Initializes module if neccessary.
unsigned int Worker_Destroy                  (Worker* self);          // Frees the worker.
Worker*      Worker_Lookup                   (const char* alias);     // Lookup by name.  Returns NULL if nothing found.
Device*      Worker_Get_Device               (Worker* self);          // Returns the device for the worker.

unsigned int Workers_Destroy_All             (void);                  // Frees all worker instances.

//
// Workers
//
Device* Worker_Compose_Averager_f32( const char *alias, Device *source, int ichan, int ntimes );
//TODO: Device* Worker_Compose_Progressive_Averager_f32( const char *alias, Device *source, int ichan );
Device* Worker_Compose_Caster( const char *alias, Device *source, Basic_Type_ID source_type, Basic_Type_ID dest_type );
