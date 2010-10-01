//---------------------------------------------------------------------------
//
// Copyright (c) 2008-2010 AlazarTech, Inc.
// 
// AlazarTech, Inc. licenses this software under specific terms and
// conditions. Use of any of the software or derviatives thereof in any
// product without an AlazarTech digitizer board is strictly prohibited. 
// 
// AlazarTech, Inc. provides this software AS IS, WITHOUT ANY WARRANTY,
// EXPRESS OR IMPLIED, INCLUDING, WITHOUT LIMITATION, ANY WARRANTY OF
// MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE. AlazarTech makes no 
// guarantee or representations regarding the use of, or the results of the 
// use of, the software and documentation in terms of correctness, accuracy,
// reliability, currentness, or otherwise; and you rely on the software,
// documentation and results solely at your own risk.
// 
// IN NO EVENT SHALL ALAZARTECH BE LIABLE FOR ANY LOSS OF USE, LOSS OF 
// BUSINESS, LOSS OF PROFITS, INDIRECT, INCIDENTAL, SPECIAL OR CONSEQUENTIAL 
// DAMAGES OF ANY KIND. IN NO EVENT SHALL ALAZARTECH'S TOTAL LIABILITY EXCEED
// THE SUM PAID TO ALAZARTECH FOR THE PRODUCT LICENSED HEREUNDER.
// 
//---------------------------------------------------------------------------

// IoBuffer.cpp : Manage IO_BUFFER structures
//

#include <stdio.h>
#include "AlazarApi.h"
#include "IoBuffer.h"

//----------------------------------------------------------------------------
//
// Function    :  CreateIoBuffer
//
// Description :  Allocate and initialize an IO_BUFFER structure
//
//----------------------------------------------------------------------------

IO_BUFFER*  CreateIoBuffer (U32 uBufferLength_bytes)
{
	BOOL bSuccess = FALSE;


	// allocate an IO_BUFFER structure

	IO_BUFFER* pIoBuffer = (IO_BUFFER*) malloc (sizeof (IO_BUFFER));
	if (pIoBuffer == NULL)
	{
		printf ("Error: Alloc IO_BUFFER failed\n");
		return NULL;
	}

	memset (pIoBuffer, 0, sizeof (IO_BUFFER));

	// allocate a buffer for sample data

	pIoBuffer->pBuffer = 
		VirtualAlloc (
			NULL,
			uBufferLength_bytes,
			MEM_COMMIT,
			PAGE_READWRITE
			);

	if (pIoBuffer->pBuffer == NULL)
	{
		printf ("Error: Alloc %lu bytes failed\n", uBufferLength_bytes);
		goto error;
	}

	pIoBuffer->uBufferLength_bytes = uBufferLength_bytes;

	// create an event 

	pIoBuffer->hEvent = 
		CreateEvent(
			NULL,	// lpEventAttributes
			TRUE,	// bManualReset
			FALSE,	// bInitialState
			NULL	// lpName
			);	

	if (pIoBuffer->hEvent == NULL)
	{
		printf ("Error: CreateEvent failed -- %u\n", GetLastError());
		goto error;
	}

	pIoBuffer->overlapped.hEvent = pIoBuffer->hEvent;

	bSuccess = TRUE;

error:

	if (!bSuccess)
	{
		DestroyIoBuffer (pIoBuffer);
		pIoBuffer = NULL;
	}

	return pIoBuffer;

}


//----------------------------------------------------------------------------
//
// Function    :  DestroyIoBuffer
//
// Description :  Release an IO_BUFFER's resources
//
//----------------------------------------------------------------------------

BOOL DestroyIoBuffer (IO_BUFFER *pIoBuffer)
{
	if (pIoBuffer != NULL)
	{
		if (pIoBuffer->bPending)
		{
			printf ("Error: Buffer is in use.\n");
			return FALSE;
		}
		else
		{
			if (pIoBuffer->hEvent != NULL)
				CloseHandle (pIoBuffer->hEvent);

			if (pIoBuffer->pBuffer != NULL)
			{
				VirtualFree (
					pIoBuffer->pBuffer, 
					0,
					MEM_RELEASE
					);
			}

			free (pIoBuffer);
		}
	}

	return TRUE;
}



//----------------------------------------------------------------------------
//
// Function    :  ResetIoBuffer
//
// Description :  Initialize an IO_BUFFER for a data transfer
//
//----------------------------------------------------------------------------

BOOL ResetIoBuffer (IO_BUFFER *pIoBuffer)
{
	if (pIoBuffer == NULL)
	{
		printf ("Error: NULL IoBuffer\n");
		return FALSE;
	}

	if (!ResetEvent (pIoBuffer->hEvent))
	{
		DWORD dwErrorCode = GetLastError ();
		printf ("Error: ResetEvent failed -- %u\n", GetLastError());
		return FALSE;
	}

	pIoBuffer->bPending = FALSE;
	pIoBuffer->overlapped.Internal = 0;
	pIoBuffer->overlapped.InternalHigh = 0;
	pIoBuffer->overlapped.Offset = 0;
	pIoBuffer->overlapped.OffsetHigh = 0;
	return TRUE;
}


