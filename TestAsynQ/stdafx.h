// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "targetver.h"

#include <stdio.h>
#include <tchar.h>



// TODO: reference additional headers your program requires here
#include "../fetch/common.h"

// debugging defines
#define DEBUG_ASYNQ_FLUSH_WAITING_CONSUMERS
#define DEBUG_ASYNQ_HANDLE_WAIT_FOR_RESULT
#define DEBUG_ASYNQ_UNREF