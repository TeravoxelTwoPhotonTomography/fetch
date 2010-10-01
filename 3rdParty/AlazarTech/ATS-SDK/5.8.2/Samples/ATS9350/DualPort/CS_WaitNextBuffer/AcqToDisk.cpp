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

// AcqToDisk.cpp :
//
// This program demonstrates how to configure an ATS9350 to make a
// continuous streaming AutoDMA acquisition. 
//

#include <stdio.h>
#include <conio.h>
#include "AlazarError.h"
#include "AlazarApi.h"
#include "AlazarCmd.h"

// Globals variables

U16* Buffer = NULL;
double SamplesPerSec = 0.;

// Forward declarations

BOOL ConfigureBoard(HANDLE boardHandle);
BOOL AcquireData(HANDLE boardHandle);

//----------------------------------------------------------------------------
//
// Function    :  main
//
// Description :  Program entry point
//
//----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
	// TODO: Select a board

	U32 systemId = 1;
	U32 boardId = 1;

	// Get a handle to the board

	HANDLE boardHandle = AlazarGetBoardBySystemID(systemId, boardId);
	if (boardHandle == NULL)
	{
		printf("Error: Unable to open board system Id %u board Id %u\n", systemId, boardId);
		return 1;
	}

	// Configure the board's sample rate, input, and trigger settings

	if (!ConfigureBoard(boardHandle))
	{
		printf("Error: Configure board failed\n");
		return 1;
	}

	// Make an acquisition, optionally saving sample data to a file

	if (!AcquireData(boardHandle))
	{
		printf("Error: Acquisition failed\n");
		return 1;
	}

	return 0;
}

//----------------------------------------------------------------------------
//
// Function    :  ConfigureBoard
//
// Description :  Configure sample rate, input, and trigger settings
//
//----------------------------------------------------------------------------

BOOL ConfigureBoard(HANDLE boardHandle)
{
	RETURN_CODE retCode;

	// TODO: Specify the sample rate (see sample rate id below)

	SamplesPerSec = 20.e6;

	// TODO: Select clock parameters as required to generate this sample rate.
	//
	// For example: if samplesPerSec is 100.e6 (100 MS/s), then:
	// - select clock source INTERNAL_CLOCK and sample rate SAMPLE_RATE_100MSPS
	// - select clock source FAST_EXTERNAL_CLOCK, sample rate SAMPLE_RATE_USER_DEF,
	//   and connect a 100 MHz signalto the EXT CLK BNC connector.

	retCode = 
		AlazarSetCaptureClock(
			boardHandle,			// HANDLE -- board handle
			INTERNAL_CLOCK,			// U32 -- clock source id
			SAMPLE_RATE_20MSPS,		// U32 -- sample rate id
			CLOCK_EDGE_RISING,		// U32 -- clock edge id
			0						// U32 -- clock decimation 
			);
	if (retCode != ApiSuccess)
	{
		printf("Error: AlazarSetCaptureClock failed -- %s\n", AlazarErrorToText(retCode));
		return FALSE;
	}

	// TODO: Select CHA input parameters as required
	
	retCode = 
		AlazarInputControl(
			boardHandle,			// HANDLE -- board handle
			CHANNEL_A,				// U8 -- input channel 
			DC_COUPLING,			// U32 -- input coupling id
			INPUT_RANGE_PM_1_V,		// U32 -- input range id
			IMPEDANCE_50_OHM		// U32 -- input impedance id
			);
	if (retCode != ApiSuccess)
	{
		printf("Error: AlazarInputControl failed -- %s\n", AlazarErrorToText(retCode));
		return FALSE;
	}

	// TODO: Select CHA bandwidth limit as required
	
	retCode = 
		AlazarSetBWLimit(
			boardHandle,			// HANDLE -- board handle
			CHANNEL_A,				// U8 -- channel identifier
			0						// U32 -- 0 = disable, 1 = enable
			);
	if (retCode != ApiSuccess)
	{
		printf("Error: AlazarSetBWLimit failed -- %s\n", AlazarErrorToText(retCode));
		return FALSE;
	}

	// TODO: Select CHB input parameters as required
	
	retCode = 
		AlazarInputControl(
			boardHandle,			// HANDLE -- board handle
			CHANNEL_B,				// U8 -- channel identifier
			DC_COUPLING,			// U32 -- input coupling id
			INPUT_RANGE_PM_1_V,		// U32 -- input range id
			IMPEDANCE_50_OHM		// U32 -- input impedance id
			);
	if (retCode != ApiSuccess)
	{
		printf("Error: AlazarInputControl failed -- %s\n", AlazarErrorToText(retCode));
		return FALSE;
	}

	// TODO: Select CHB bandwidth limit as required
	
	retCode = 
		AlazarSetBWLimit(
			boardHandle,			// HANDLE -- board handle
			CHANNEL_B,				// U8 -- channel identifier
			0						// U32 -- 0 = disable, 1 = enable
			);
	if (retCode != ApiSuccess)
	{
		printf("Error: AlazarSetBWLimit failed -- %s\n", AlazarErrorToText(retCode));
		return FALSE;
	}

	// TODO: Select trigger inputs and levels as required

	retCode = 
		AlazarSetTriggerOperation(
			boardHandle,			// HANDLE -- board handle
			TRIG_ENGINE_OP_J,		// U32 -- trigger operation 
			TRIG_ENGINE_J,			// U32 -- trigger engine id
			TRIG_CHAN_A,			// U32 -- trigger source id
			TRIGGER_SLOPE_POSITIVE,	// U32 -- trigger slope id
			128,					// U32 -- trigger level from 0 (-range) to 255 (+range)
			TRIG_ENGINE_K,			// U32 -- trigger engine id
			TRIG_DISABLE,			// U32 -- trigger source id for engine K
			TRIGGER_SLOPE_POSITIVE,	// U32 -- trigger slope id
			128						// U32 -- trigger level from 0 (-range) to 255 (+range)
			);
	if (retCode != ApiSuccess)
	{
		printf("Error: AlazarSetTriggerOperation failed -- %s\n", AlazarErrorToText(retCode));
		return FALSE;
	}

	// TODO: Select external trigger parameters as required

	retCode =
		AlazarSetExternalTrigger( 
			boardHandle,			// HANDLE -- board handle
			DC_COUPLING,			// U32 -- external trigger coupling id
			ETR_5V					// U32 -- external trigger range id
			);

	// TODO: Set trigger delay as required. 
	
	double triggerDelay_sec = 0.;
	U32 triggerDelay_samples = (U32) (triggerDelay_sec * SamplesPerSec + 0.5);
	retCode = AlazarSetTriggerDelay(boardHandle, triggerDelay_samples);
	if (retCode != ApiSuccess)
	{
		printf("Error: AlazarSetTriggerDelay failed -- %s\n", AlazarErrorToText(retCode));
		return FALSE;
	}

	// TODO: Set trigger timeout as required. 

	// NOTE:
	// The board will wait for a for this amount of time for a trigger event. 
	// If a trigger event does not arrive, then the board will automatically 
	// trigger. Set the trigger timeout value to 0 to force the board to wait 
	// forever for a trigger event.
	//
	// IMPORTANT: 
	// The trigger timeout value should be set to zero after appropriate 
	// trigger parameters have been determined, otherwise the 
	// board may trigger if the timeout interval expires before a 
	// hardware trigger event arrives.

	double triggerTimeout_sec = 0.;
	U32 triggerTimeout_clocks = (U32) (triggerTimeout_sec / 10.e-6 + 0.5);

	retCode = 
		AlazarSetTriggerTimeOut(
			boardHandle,			// HANDLE -- board handle
			triggerTimeout_clocks	// U32 -- timeout_sec / 10.e-6 (0 means wait forever)
			);
	if (retCode != ApiSuccess)
	{
		printf("Error: AlazarSetTriggerTimeOut failed -- %s\n", AlazarErrorToText(retCode));
		return FALSE;
	}

	// TODO: Configure AUX I/O connector as required

	retCode = 
		AlazarConfigureAuxIO(
			boardHandle,			// HANDLE -- board handle
			AUX_OUT_TRIGGER,		// U32 -- mode
			0						// U32 -- parameter
			);	
	if (retCode != ApiSuccess)
	{
		printf("Error: AlazarConfigureAuxIO failed -- %s\n", AlazarErrorToText(retCode));
		return FALSE;
	}

	return TRUE;
}

//----------------------------------------------------------------------------
//
// Function    :  AcquireData
//
// Description :  Perform an acquisition, optionally saving data to file.
//
//----------------------------------------------------------------------------

BOOL AcquireData(HANDLE boardHandle)
{
	// TODO: Select the total acquisition length in seconds

	double acquisitionLength_sec = 10.;

	// TODO: Select the number of samples in each DMA transfer

	U32 samplesPerBuffer = 1024 * 1024;

	// TODO: Select which channels to capture (A, B, or both)

	U32 channelMask = CHANNEL_A | CHANNEL_B; 

	// TODO: Select if you wish to save the sample data to a file

	BOOL saveData = FALSE;

	// Calculate the number of enabled channels from the channel mask 

	int channelCount = 0;

	switch (channelMask)
	{
	case CHANNEL_A:
	case CHANNEL_B:
		channelCount = 1;
		break;
	case CHANNEL_A | CHANNEL_B:
		channelCount = 2;
		break;
	default:
		printf("Error: Invalid channel mask %08X\n", channelMask);
		return FALSE;
	}

	// Detemine the sample size in bits, and the on-board memory size in samples per channel

	U8 bitsPerSample;
	U32 maxSamplesPerChannel;
	RETURN_CODE retCode = AlazarGetChannelInfo(boardHandle, &maxSamplesPerChannel, &bitsPerSample);
	if (retCode != ApiSuccess)
	{
		printf("Error: AlazarGetChannelInfo failed -- %s\n", AlazarErrorToText(retCode));
		return FALSE;
	}

	// Calculate the size of each DMA buffer in bytes

	U32 bytesPerSample = (bitsPerSample + 7) / 8;
	U32 bytesPerBuffer = bytesPerSample * samplesPerBuffer * channelCount;

	// Calculate the number of buffers in the acquisition

	U32 samplesPerAcquisition = (U32) (SamplesPerSec * acquisitionLength_sec + 0.5);
	U32 buffersPerAcquisition = (samplesPerAcquisition + samplesPerBuffer - 1) / samplesPerBuffer;

	// Create a data file if required

	FILE *fpData = NULL;

	if (saveData)
	{
		fpData = fopen("data.bin", "wb");
		if (fpData == NULL)
		{
			printf("Error: Unable to create data file -- %u\n", GetLastError());
			free(Buffer);
			return FALSE;
		}
	}

	// Allocate memory for DMA buffers

	BOOL success = TRUE;

	Buffer = (U16 *) malloc(bytesPerBuffer); 
	if (Buffer == NULL)
	{
		printf("Error: alloc %d bytes failed\n", bytesPerBuffer);
		success = FALSE;
	}
			
	// Configure the board to make an AutoDMA acquisition

	if (success)
	{
		U32 admaFlags = ADMA_EXTERNAL_STARTCAPTURE | // Start acquisition when we call AlazarStartCapture 
						ADMA_CONTINUOUS_MODE |		 // Acquire a continuous stream of sample data without trigger
						ADMA_ALLOC_BUFFERS;			 // Allow API manages DMA buffers, and copies data into our buffer.

		retCode = 
			AlazarBeforeAsyncRead(
				boardHandle,			// HANDLE -- board handle
				channelMask,			// U32 -- enabled channel mask
				0,						// long -- offset from trigger in samples
				samplesPerBuffer,		// U32 -- samples per buffer
				1,						// U32 -- records per buffer (must be 1)
				buffersPerAcquisition,	// U32 -- records per acquisition 
				admaFlags				// U32 -- AutoDMA flags
				); 
		if (retCode != ApiSuccess)
		{
			printf("Error: AlazarBeforeAsyncRead failed -- %s\n", AlazarErrorToText(retCode));
			success = FALSE;
		}
	}

	// Arm the board to begin the acquisition 

	if (success)
	{
		retCode = AlazarStartCapture(boardHandle);
		if (retCode != ApiSuccess)
		{
			printf("Error: AlazarStartCapture failed -- %s\n", AlazarErrorToText(retCode));
			success = FALSE;
		}
	}
	
	// Wait for the API to fill a buffer and copy the data into our buffer

	if (success)
	{
		printf("Capturing %d buffers ... press any key to abort\n", buffersPerAcquisition);

		DWORD startTickCount = GetTickCount();
		U32 buffersCompleted = 0;
		INT64 bytesTransferred = 0;
		
		while (buffersCompleted < buffersPerAcquisition)
		{
			// TODO: Set a buffer timeout that is longer than the time 
			//       required to capture all the records in one buffer.

			DWORD timeout_ms = 5000;

			// Wait for a buffer to be filled by the board.

			retCode = AlazarWaitNextAsyncBufferComplete(boardHandle, Buffer, bytesPerBuffer, timeout_ms);
			if (retCode == ApiSuccess)
			{
				// This buffer is complete, but there are more buffers in the acquisition.
			}
			else if (retCode == ApiTransferComplete)
			{
				// This buffer is complete, and it is the last buffer of the acqusition.
			}
			else
			{
				printf("Error: AlazarWaitNextAsyncBufferComplete failed -- %s\n", AlazarErrorToText(retCode));
				success = FALSE;
			}

			if (success)
			{
				// This buffer is full and has been removed from the list
				// of buffers available to the board.

				buffersCompleted++;
				bytesTransferred += bytesPerBuffer;

				// TODO: Process sample data in this buffer. 

				// NOTE: 
				//
				// While you are processing this buffer, the board is already
				// filling the next available buffer(s). 
				// 
				// You MUST finish processing this buffer and post it back to the 
				// board before the board fills all of the available DMA buffers, 
				// and its on-board memory. 
				// 
				// Records are arranged in the buffer as follows: R0A, R0B
				//
				// Samples are arranged contiguously in each record.
				// A 12-bit sample code occupies the most significant bits of 
				// each 16-bit sample value.
				//
				// Sample codes are unsigned by default. As a result:
				// - a sample code of 0x000 represents a negative full scale input signal.
				// - a sample code of 0x800 represents a ~0V signal.
				// - a sample code of 0xFFF represents a positive full scale input signal.

				if (saveData)
				{
					// Write buffer to file

					size_t bytesWritten = fwrite(Buffer, sizeof(BYTE), bytesPerBuffer, fpData);
					if (bytesWritten != bytesPerBuffer)
					{
						printf("Error: Write buffer %d failed -- %u\n", buffersCompleted, GetLastError());
						success = FALSE;
					}
				}
			}

			// If an error occured, exit the acquisition loop

			if (!success)
				break;
				
			// If a key was pressed, exit the acquisition loop
				
			if (_kbhit())
			{
				printf("Aborted...\n");
				break;
			}

			// Display progress

			printf("Completed %u buffers\r", buffersCompleted);
		}

		// Display results

		double transferTime_sec = (GetTickCount() - startTickCount) / 1000.;
		printf("Capture completed in %.2lf sec\n", transferTime_sec);

		double buffersPerSec;
		double bytesPerSec;
		if (transferTime_sec > 0.)
		{
			buffersPerSec = buffersCompleted / transferTime_sec;
			bytesPerSec = bytesTransferred / transferTime_sec;
		}
		else
		{
			buffersPerSec = 0.;
			bytesPerSec = 0.;
		}

		printf("Captured %d buffers (%.4g buffers per sec)\n", buffersCompleted, buffersPerSec);
		printf("Transferred %I64d bytes (%.4g bytes per sec)\n", bytesTransferred, bytesPerSec);
	}

	// Abort the acquisition

	retCode = AlazarAbortAsyncRead(boardHandle);
	if (retCode != ApiSuccess)
	{
		printf("Error: AlazarAbortAsyncRead failed -- %s\n", AlazarErrorToText(retCode));
		success = FALSE;
	}

	// Free all memory allocated

	free(Buffer);

	// Close the data file

	if (fpData != NULL)
		fclose(fpData);	
	
	return success;
}

