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

using System;
using System.IO;
using System.Runtime.InteropServices;
using AlazarTech;

namespace AlazarSysInfo
{
    // This program demonstrates how to configure an ATS310 to acquire to 
    // on-board memory, and use AlazarRead to transfer records from on-board memory
    // to an application buffer.

    class AlazarSysInfoApp
    {
        static unsafe void Main(string[] args)
        {
            // Display SDK version

	        byte sdkMajor, sdkMinor, sdkRevision;
	        UInt32 retCode = AlazarAPI.AlazarGetSDKVersion(&sdkMajor, &sdkMinor, &sdkRevision);
	        if (retCode != AlazarAPI.ApiSuccess)
	        {
		        Console.WriteLine("Error: AlazarGetSDKVersion failed -- " + 
                    AlazarAPI.AlazarErrorToText(retCode));
		        return;
	        }

            Console.WriteLine("SDK version               = {1}.{2}.{3}", sdkMajor, sdkMinor, sdkRevision);

            // Display information about each board system

	        UInt32 systemCount = AlazarAPI.AlazarNumOfSystems();
            if (systemCount < 1)
            {
                Console.WriteLine("No systems found!");
                return;
            }

	        Console.WriteLine("System count              = {1}", systemCount);

	        for (UInt32 systemId = 1; systemId <= systemCount; systemId++)
	        {
		        if (!DisplaySystemInfo(systemId))
			        return;
	        }

	        return;
        }

        //---------------------------------------------------------------------------
        //
        // Function    :  DisplaySystemInfo
        //
        // Description :  Display information about this board system
        //
        //---------------------------------------------------------------------------

        static public unsafe bool DisplaySystemInfo(UInt32 systemId)
        {
	        UInt32 boardCount = AlazarAPI.AlazarBoardsInSystemBySystemID(systemId);
	        if (boardCount == 0)
	        {
		        Console.WriteLine("Error: No boards found in system.");
		        return false;
	        }

	        IntPtr handle = AlazarAPI.AlazarGetSystemHandle(systemId);
	        if (handle == IntPtr.Zero)
	        {
		        Console.WriteLine("Error: AlazarGetSystemHandle system failed.");
		        return false;
	        }

	        UInt32 boardType = AlazarAPI.AlazarGetBoardKind(handle);
	        if (boardType == AlazarAPI.ATS_NONE || boardType >= AlazarAPI.ATS_LAST)
	        {
		        Console.WriteLine("Error: Unknown board type " + boardType);
		        return false;
	        }

	        byte driverMajor, driverMinor, driverRev;
	        UInt32 retCode = AlazarAPI.AlazarGetDriverVersion(&driverMajor, &driverMinor, &driverRev);
	        if (retCode != AlazarAPI.ApiSuccess)
	        {
		        Console.WriteLine("Error: AlazarGetDriverVersion failed -- " + 
                    AlazarAPI.AlazarErrorToText(retCode));
		        return false;
	        }

	        Console.WriteLine("System ID                 = {0}", systemId);
	        Console.WriteLine("Board type                = {0}", BoardTypeToText(boardType));
	        Console.WriteLine("Board count               = {0}", boardCount);
	        Console.WriteLine("Driver version            = {0}.{1}.{2}", driverMajor, driverMinor, driverRev);
	        Console.WriteLine("");

	        // Display informataion about each board in this board system

	        for (UInt32 boardId = 1; boardId <= boardCount; boardId++)
	        {
		        if (!DisplayBoardInfo(systemId, boardId))
			        return false;
	        }

	        return true;
        }

        //---------------------------------------------------------------------------
        //
        // Function    :  DisplayBoardInfo
        //
        // Description :  Display information about a board
        //
        //---------------------------------------------------------------------------

        static public unsafe bool DisplayBoardInfo(UInt32 systemId, UInt32 boardId)
        {
	        UInt32 retCode;

            IntPtr handle = AlazarAPI.AlazarGetBoardBySystemID(systemId, boardId);
            if (handle == IntPtr.Zero)
            {
                Console.WriteLine("Error: Open systemId {0} boardId {1} failed", systemId, boardId);
		        return false;
            }

	        UInt32 samplesPerChannel;
	        byte bitsPerSample;
	        retCode = AlazarAPI.AlazarGetChannelInfo(handle, &samplesPerChannel, &bitsPerSample);
	        if (retCode != AlazarAPI.ApiSuccess)
	        {
		        Console.WriteLine("Error: AlazarGetChannelInfo failed -- " + 
                    AlazarAPI.AlazarErrorToText(retCode));
		        return false;
	        }

	        UInt32 aspocType;
            retCode = AlazarAPI.AlazarQueryCapability(handle, AlazarAPI.ASOPC_TYPE, 0, &aspocType);
            if (retCode != AlazarAPI.ApiSuccess)
            {
                Console.WriteLine("Error: AlazarQueryCapability failed -- " +
                    AlazarAPI.AlazarErrorToText(retCode));
		        return false;
            }

	        byte cpldMajor;
	        byte cpldMinor;
	        retCode = AlazarAPI.AlazarGetCPLDVersion(handle, &cpldMajor, &cpldMinor);
	        if (retCode != AlazarAPI.ApiSuccess)
	        {
		        Console.WriteLine("Error: AlazarGetCPLDVersion failed -- " + 
                    AlazarAPI.AlazarErrorToText(retCode));
		        return false;
	        }

	        UInt32 serialNumber;
	        retCode = AlazarAPI.AlazarQueryCapability(handle, AlazarAPI.GET_SERIAL_NUMBER, 0, &serialNumber);
	        if (retCode != AlazarAPI.ApiSuccess)
	        {
                Console.WriteLine("Error: AlazarQueryCapability failed -- " +
                    AlazarAPI.AlazarErrorToText(retCode));
		        return false;		
	        }

	        UInt32 latestCalDate;
	        retCode = AlazarAPI.AlazarQueryCapability(handle, AlazarAPI.GET_LATEST_CAL_DATE, 0, &latestCalDate);
	        if (retCode != AlazarAPI.ApiSuccess)
	        {
                Console.WriteLine("Error: AlazarQueryCapability failed -- " + 
                    AlazarAPI.AlazarErrorToText(retCode));
		        return false;
	        }

	        Console.WriteLine("System ID                 = {0}", systemId);
	        Console.WriteLine("Board ID                  = {0}", boardId);
	        Console.WriteLine("Serial number             = {0}", serialNumber);
	        Console.WriteLine("Bits per sample           = {0}", bitsPerSample);
	        Console.WriteLine("Max samples per channel   = {0}", samplesPerChannel);
	        Console.WriteLine("CPLD version              = {0}.{1}", cpldMajor, cpldMinor);
	        Console.WriteLine("FPGA version              = {0}", (aspocType >> 16) & 0xff, (aspocType >> 24) & 0xf);
	        Console.WriteLine("ASoPC signature           = {0}", aspocType);
	        Console.WriteLine("Latest calibration date   = {0}", latestCalDate);

	        if (IsPcieDevice(handle))
	        {
		        // Display PCI Express link information

		        UInt32 linkSpeed;
		        retCode = AlazarAPI.AlazarQueryCapability(handle, AlazarAPI.GET_PCIE_LINK_SPEED, 0, &linkSpeed);
		        if (retCode != AlazarAPI.ApiSuccess)
		        {
			        Console.WriteLine("Error: AlazarQueryCapability failed -- " +
                        AlazarAPI.AlazarErrorToText(retCode));
			        return false;
		        }

		        UInt32 linkWidth;
		        retCode = AlazarAPI.AlazarQueryCapability(handle, AlazarAPI.GET_PCIE_LINK_WIDTH, 0, &linkWidth);
		        if (retCode != AlazarAPI.ApiSuccess)
		        {
			        Console.WriteLine("Error: AlazarQueryCapability failed -- " +
                        AlazarAPI.AlazarErrorToText(retCode));
			        return false;
		        }

		        Console.WriteLine("PCIe link speed           = {0} Gbps", 2.5 * linkSpeed);
		        Console.WriteLine("PCIe link width           = {0} lanes", linkWidth);
	        }

	        Console.WriteLine("");

	        // Toggling the LED on the PCIe/PCIe mounting bracket of the board

	        int cycleCount = 2;
	        int cyclePeriod_ms = 200;

	        if (!FlashLed(handle, cycleCount, cyclePeriod_ms))
		        return false;

	        return true;
        }

        //---------------------------------------------------------------------------
        //
        // Function    :  IsPcieDevice
        //
        // Description :  Return TRUE if board has PCIe host bus interface
        //
        //---------------------------------------------------------------------------

        static public bool IsPcieDevice(IntPtr handle)
        {
	        UInt32 boardType = AlazarAPI.AlazarGetBoardKind(handle);
	        if (boardType >= AlazarAPI.ATS9462)
		        return true;
	        else
		        return false;
        }

        //---------------------------------------------------------------------------
        //
        // Function    :  FlashLed
        //
        // Description :  Flash LED on board's PCI/PCIe mounting bracket
        //
        //---------------------------------------------------------------------------

        static public bool FlashLed(IntPtr handle, int cycleCount, int cyclePeriod_ms)
        {
	        for (int cycle = 0; cycle < cycleCount; cycle++)
	        {
		        const int phaseCount = 2;
		        int sleepPeriod_ms = cyclePeriod_ms / phaseCount;

		        for (int phase = 0; phase < phaseCount; phase++)
		        {
			        UInt32 state = (phase == 0) ? AlazarAPI.LED_ON : AlazarAPI.LED_OFF;
			        UInt32 retCode = AlazarAPI.AlazarSetLED(handle, state);
			        if (retCode != AlazarAPI.ApiSuccess)
				        return false;

			        if (sleepPeriod_ms > 0)
				        System.Threading.Thread.Sleep(sleepPeriod_ms);
		        }
	        }

	        return true;
        }

        //---------------------------------------------------------------------------
        //
        // Function    :  BoardTypeToText
        //
        // Description :  Convert board type Id to text
        //
        //---------------------------------------------------------------------------

        static public string BoardTypeToText(UInt32 boardType) 
        {
	        string name;

	        switch (boardType)
	        {
	        case AlazarAPI.ATS850:
		        name = "ATS850";
		        break;
	        case AlazarAPI.ATS310:
		        name = "ATS310";
		        break;
	        case AlazarAPI.ATS330:
		        name = "ATS330";
		        break;
	        case AlazarAPI.ATS855:
		        name = "ATS855";
		        break;
	        case AlazarAPI.ATS315:
		        name = "ATS315";
		        break;
	        case AlazarAPI.ATS335:
		        name = "ATS335";
		        break;
	        case AlazarAPI.ATS460:
		        name = "ATS460";
		        break;
	        case AlazarAPI.ATS860:
		        name = "ATS860";
		        break;
	        case AlazarAPI.ATS660:
		        name = "ATS660";
		        break;
	        case AlazarAPI.ATS9462:
		        name = "ATS9462";
		        break;
	        case AlazarAPI.ATS9870:
		        name = "ATS9870";
		        break;
	        case AlazarAPI.ATS9350:
		        name = "ATS9350";
		        break;
            default:
		        name = "?";
                break;
	        }

	        return name;
        }
    }
}
