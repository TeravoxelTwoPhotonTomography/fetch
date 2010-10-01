// AlazarSysInfo.cpp : Defines the entry point for the console application.

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

#include <stdio.h>
#include "AlazarError.h"
#include "AlazarApi.h"
#include "AlazarCmd.h"

// Forward declarations

BOOL DisplaySystemInfo(U32 systemId);
BOOL DisplayBoardInfo(U32 systemId, U32 boardId);
BOOL FlashLed(HANDLE handle, int cycleCount, DWORD cyclePeriod_ms);
BOOL IsPcieDevice(HANDLE handle);
const char* BoardTypeToText(int boardType);

//---------------------------------------------------------------------------
//
// Function    :  main
//
// Description :  Program entry point
//
//---------------------------------------------------------------------------

int main(int argc, char* argv[])
{
	// Display information about all board systems

	U8 sdkMajor, sdkMinor, sdkRevision;
	RETURN_CODE retCode = AlazarGetSDKVersion(&sdkMajor, &sdkMinor, &sdkRevision);
	if (retCode != ApiSuccess)
	{
		printf("Error: AlazarGetSDKVersion failed -- %d (%s)", retCode, AlazarErrorToText(retCode));
		return 1;
	}

	U32 systemCount = AlazarNumOfSystems();

	printf("SDK version               = %d.%d.%d\n", sdkMajor, sdkMinor, sdkRevision);
	printf("System count              = %u\n", systemCount);
	printf("\n");

	// Display informataion about each board system 

	if (systemCount < 1)
	{
		printf("No systems found!\n");
	}
	else
	{
		for (U32 systemId = 1; systemId <= systemCount; systemId++)
		{
			if (!DisplaySystemInfo(systemId))
				return 1;
		}
	}

	return 0;
}

//---------------------------------------------------------------------------
//
// Function    :  DisplaySystemInfo
//
// Description :  Display information about this board system
//
//---------------------------------------------------------------------------

BOOL DisplaySystemInfo(U32 systemId)
{
	U32 boardCount = AlazarBoardsInSystemBySystemID(systemId);
	if (boardCount == 0)
	{
		printf("Error: No boards found in system.\n");
		return FALSE;
	}

	HANDLE handle = AlazarGetSystemHandle(systemId);
	if (handle == NULL)
	{
		printf("Error: AlazarGetSystemHandle system failed.\n");
		return FALSE;
	}

	U32 boardType = AlazarGetBoardKind(handle);
	if (boardType == ATS_NONE || boardType >= ATS_LAST)
	{
		printf("Error: Unknown board type %u\n", boardType);
		return FALSE;
	}

	U8 driverMajor, driverMinor, driverRev;
	RETURN_CODE retCode = AlazarGetDriverVersion(&driverMajor, &driverMinor, &driverRev);
	if (retCode != ApiSuccess)
	{
		printf("Error: AlazarGetDriverVersion failed -- %s\n", AlazarErrorToText(retCode));
		return FALSE;
	}

	printf("System ID                 = %u\n", systemId);
	printf("Board type                = %s\n", BoardTypeToText(boardType));
	printf("Board count               = %u\n", boardCount);
	printf("Driver version            = %d.%d.%d\n", driverMajor, driverMinor, driverRev);
	printf("\n");

	// Display informataion about each board in this board system

	for (U32 boardId = 1; boardId <= boardCount; boardId++)
	{
		if (!DisplayBoardInfo(systemId, boardId))
			return FALSE;
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
// Function    :  DisplayBoardInfo
//
// Description :  Display information about a board
//
//---------------------------------------------------------------------------

BOOL DisplayBoardInfo(U32 systemId, U32 boardId)
{
	RETURN_CODE retCode;

    HANDLE handle = AlazarGetBoardBySystemID(systemId, boardId);
    if (handle == NULL)
    {
        printf("Error: Open systemId %d boardId %u failed\n", systemId, boardId);
		return FALSE;
    }

	U32 samplesPerChannel;
	BYTE bitsPerSample;
	retCode = AlazarGetChannelInfo(handle, &samplesPerChannel, &bitsPerSample);
	if (retCode != ApiSuccess)
	{
		printf("Error: AlazarGetChannelInfo failed -- %s\n", AlazarErrorToText(retCode));
		return FALSE;
	}

	U32 aspocType;
    retCode = AlazarQueryCapability(handle, ASOPC_TYPE, 0, &aspocType);
    if (retCode != ApiSuccess)
    {
        printf("Error: AlazarQueryCapability failed -- %s.\n", AlazarErrorToText(retCode));
		return FALSE;
    }

	BYTE cpldMajor;
	BYTE cpldMinor;
	retCode = AlazarGetCPLDVersion(handle, &cpldMajor, &cpldMinor);
	if (retCode != ApiSuccess)
	{
		printf("Error: AlazarGetCPLDVersion failed -- %s", AlazarErrorToText(retCode));
		return FALSE;
	}

	U32 serialNumber;
	retCode = AlazarQueryCapability(handle, GET_SERIAL_NUMBER, 0, &serialNumber);
	if (retCode != ApiSuccess)
	{
        printf("Error: AlazarQueryCapability failed -- %s.\n", AlazarErrorToText(retCode));
		return FALSE;		
	}

	U32 latestCalDate;
	retCode = AlazarQueryCapability(handle, GET_LATEST_CAL_DATE, 0, &latestCalDate);
	if (retCode != ApiSuccess)
	{
        printf("Error: AlazarQueryCapability failed -- %s.\n", AlazarErrorToText(retCode));
		return FALSE;
	}

	printf("System ID                 = %u\n", systemId);
	printf("Board ID                  = %u\n", boardId);
	printf("Serial number             = %06d\n", serialNumber);
	printf("Bits per sample           = %d\n", bitsPerSample);
	printf("Max samples per channel   = %u\n", samplesPerChannel);
	printf("CPLD version              = %d.%d\n", cpldMajor, cpldMinor);
	printf("FPGA version              = %d.%d\n", (aspocType >> 16) & 0xff, (aspocType >> 24) & 0xf);
	printf("ASoPC signature           = 0x%08X\n", aspocType);
	printf("Latest calibration date   = %d\n", latestCalDate);

	if (IsPcieDevice(handle))
	{
		// Display PCI Express link information

		U32 linkSpeed;
		retCode = AlazarQueryCapability(handle, GET_PCIE_LINK_SPEED, 0, &linkSpeed);
		if (retCode != ApiSuccess)
		{
			printf("Error: AlazarQueryCapability failed -- %s.\n", AlazarErrorToText(retCode));
			return FALSE;
		}

		U32 linkWidth;
		retCode = AlazarQueryCapability(handle, GET_PCIE_LINK_WIDTH, 0, &linkWidth);
		if (retCode != ApiSuccess)
		{
			printf("Error: AlazarQueryCapability failed -- %s.\n", AlazarErrorToText(retCode));
			return FALSE;
		}

		printf("PCIe link speed           = %g Gbps\n", 2.5 * linkSpeed);
		printf("PCIe link width           = %u lanes\n", linkWidth);
	}

	printf("\n");

	// Toggling the LED on the PCIe/PCIe mounting bracket of the board

	int cycleCount = 2;
	DWORD cyclePeriod_ms = 200;

	if (!FlashLed(handle, cycleCount, cyclePeriod_ms))
		return FALSE;

	return TRUE;
}

//---------------------------------------------------------------------------
//
// Function    :  IsPcieDevice
//
// Description :  Return TRUE if board has PCIe host bus interface
//
//---------------------------------------------------------------------------

BOOL IsPcieDevice(HANDLE handle)
{
	U32 boardType = AlazarGetBoardKind(handle);
	if (boardType >= ATS9462)
		return TRUE;
	else
		return FALSE;
}

//---------------------------------------------------------------------------
//
// Function    :  FlashLed
//
// Description :  Flash LED on board's PCI/PCIe mounting bracket
//
//---------------------------------------------------------------------------

BOOL FlashLed(HANDLE handle, int cycleCount, DWORD cyclePeriod_ms)
{
	for (int cycle = 0; cycle < cycleCount; cycle++)
	{
		const int phaseCount = 2;
		DWORD sleepPeriod_ms = cyclePeriod_ms / phaseCount;

		for (int phase = 0; phase < phaseCount; phase++)
		{
			U32 state = (phase == 0) ? LED_ON : LED_OFF;
			RETURN_CODE retCode = AlazarSetLED(handle, state);
			if (retCode != ApiSuccess)
				return FALSE;

			if (sleepPeriod_ms > 0)
				Sleep(sleepPeriod_ms);
		}
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
// Function    :  BoardTypeToText
//
// Description :  Convert board type Id to text
//
//---------------------------------------------------------------------------

const char* BoardTypeToText(int boardType) 
{
	char* pszName;

	switch (boardType)
	{
	case ATS850:
		pszName = "ATS850";
		break;
	case ATS310:
		pszName = "ATS310";
		break;
	case ATS330:
		pszName = "ATS330";
		break;
	case ATS855:
		pszName = "ATS855";
		break;
	case ATS315:
		pszName = "ATS315";
		break;
	case ATS335:
		pszName = "ATS335";
		break;
	case ATS460:
		pszName = "ATS460";
		break;
	case ATS860:
		pszName = "ATS860";
		break;
	case ATS660:
		pszName = "ATS660";
		break;
	case ATS9462:
		pszName = "ATS9462";
		break;
	case ATS9870:
		pszName = "ATS9870";
		break;
	case ATS9350:
		pszName = "ATS9350";
		break;
	case ATS_NONE:
	default:
		pszName = "?";
	}

	return pszName;
}
