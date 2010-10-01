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

#ifndef __IO_BUFFER_H__
#define __IO_BUFFER_H__

typedef struct _IO_BUFFER
{
	void	   *pBuffer;
	OVERLAPPED	overlapped;
	HANDLE      hEvent;
	U32		    uTransferLength_bytes;
	U32 		uBufferLength_bytes;
	BOOL		bPending;
} IO_BUFFER, *PIO_BUFFER;


IO_BUFFER* 
CreateIoBuffer (
	U32 uBufferLength_bytes
	);

BOOL
DestroyIoBuffer (
	IO_BUFFER *pIoBuffer
	);

BOOL 
ResetIoBuffer (
	IO_BUFFER *pIoBuffer
	);

#endif