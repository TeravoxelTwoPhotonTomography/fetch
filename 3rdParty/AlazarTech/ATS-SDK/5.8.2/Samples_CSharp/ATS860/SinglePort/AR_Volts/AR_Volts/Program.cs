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

namespace AR_Volts
{
    // This program demonstrates how to configure an ATS860 to acquire to 
    // on-board memory, and use AlazarRead to transfer records from on-board memory
    // to an application buffer.

    class AcqToDiskApp
    {
        static private UInt32 inputRangeIdChA = 0;
        static private UInt32 inputRangeIdChB = 0;

        static void Main(string[] args)
        {
            // TODO: Select a board
            UInt32 systemId = 1;
            UInt32 boardId = 1;

            // Get a handle to the board
            IntPtr handle = AlazarAPI.AlazarGetBoardBySystemID(systemId, boardId);
            if (handle == IntPtr.Zero)
            {
                Console.WriteLine("Error: Open board {0}:{1} failed.", systemId, boardId);
                return;
            }

            // Configure sample rate, input, and trigger parameters
            if (!ConfigureBoard(handle))
            {
                Console.WriteLine("Error: Configure board {0}:{1} failed", systemId, boardId);
                return;
            }

            // Acquire data from the board to an application buffer,
            // optionally saving the data to file
            if (!AcquireData(handle))
            {
                Console.WriteLine("Error: Acquire from board {0}:{1} failed", systemId, boardId);
                return;
            }
        }

        //----------------------------------------------------------------------------
        //
        // Function    :  ConfigureBoard
        //
        // Description :  Configure sample rate, input, and trigger settings
        //
        //----------------------------------------------------------------------------

        static public bool ConfigureBoard(IntPtr boardHandle)
        {
            UInt32 retCode;

            // TODO: Specify the sample rate (in samples per second), 
            //       and appropriate sample rate identifier

            double samplesPerSec = 10e6;
            UInt32 sampleRateId = AlazarAPI.SAMPLE_RATE_10MSPS;

            // TODO: Select clock parameters as required.

            retCode =
                AlazarAPI.AlazarSetCaptureClock(
                    boardHandle,	                // HANDLE -- board handle
                    AlazarAPI.INTERNAL_CLOCK,		// U32 -- clock source id
                    sampleRateId,		            // U32 -- sample rate id
                    AlazarAPI.CLOCK_EDGE_RISING,	// U32 -- clock edge id
                    0						        // U32 -- clock decimation 
                    );
            if (retCode != AlazarAPI.ApiSuccess)
            {
                Console.WriteLine("Error: AlazarSetCaptureClock failed -- " +
                    AlazarAPI.AlazarErrorToText(retCode));
                return false;
            }

            // TODO: Select CHA input parameters as required

            inputRangeIdChA = AlazarAPI.INPUT_RANGE_PM_1_V;

            retCode =
                AlazarAPI.AlazarInputControl(
                    boardHandle,		                // HANDLE -- board handle
                    AlazarAPI.CHANNEL_A,				// U8 -- input channel 
                    AlazarAPI.DC_COUPLING,			    // U32 -- input coupling id
                    inputRangeIdChA,	                // U32 -- input range id
                    AlazarAPI.IMPEDANCE_50_OHM		    // U32 -- input impedance id
                    );
            if (retCode != AlazarAPI.ApiSuccess)
            {
                Console.WriteLine("Error: AlazarInputControl failed -- " +
                    AlazarAPI.AlazarErrorToText(retCode));
                return false;
            }

            // TODO: Select CHA bandwidth limit as required

            retCode =
                AlazarAPI.AlazarSetBWLimit(
                    boardHandle,		            // HANDLE -- board handle
                    AlazarAPI.CHANNEL_A,		    // U8 -- channel identifier
                    0						        // U32 -- 0 = disable, 1 = enable
                    );
            if (retCode != AlazarAPI.ApiSuccess)
            {
                Console.WriteLine("Error: AlazarSetBWLimit failed -- " +
                    AlazarAPI.AlazarErrorToText(retCode));
                return false;
            }

            // TODO: Select CHB input parameters as required

            inputRangeIdChA = AlazarAPI.INPUT_RANGE_PM_1_V;

            retCode =
                AlazarAPI.AlazarInputControl(
                    boardHandle,		                // HANDLE -- board handle
                    AlazarAPI.CHANNEL_B,				// U8 -- channel identifier
                    AlazarAPI.DC_COUPLING,			    // U32 -- input coupling id
                    inputRangeIdChB,	                // U32 -- input range id
                    AlazarAPI.IMPEDANCE_50_OHM		    // U32 -- input impedance id
                    );
            if (retCode != AlazarAPI.ApiSuccess)
            {
                Console.WriteLine("Error: AlazarInputControl failed -- " +
                    AlazarAPI.AlazarErrorToText(retCode));
                return false;
            }

            // TODO: Select CHB bandwidth limit as required

            retCode =
                AlazarAPI.AlazarSetBWLimit(
                    boardHandle,		                // HANDLE -- board handle
                    AlazarAPI.CHANNEL_B,				// U8 -- channel identifier
                    0						            // U32 -- 0 = disable, 1 = enable
                    );
            if (retCode != AlazarAPI.ApiSuccess)
            {
                Console.WriteLine("Error: AlazarSetBWLimit failed -- " +
                    AlazarAPI.AlazarErrorToText(retCode));
                return false;
            }

            // TODO: Select trigger inputs and levels as required

            retCode =
                AlazarAPI.AlazarSetTriggerOperation(
                    boardHandle,		                // HANDLE -- board handle
                    AlazarAPI.TRIG_ENGINE_OP_J,		    // U32 -- trigger operation 
                    AlazarAPI.TRIG_ENGINE_J,			// U32 -- trigger engine id
                    AlazarAPI.TRIG_CHAN_A,			    // U32 -- trigger source id
                    AlazarAPI.TRIGGER_SLOPE_POSITIVE,	// U32 -- trigger slope id
                    128,        					    // U32 -- trigger level from 0 (-range) to 255 (+range)
                    AlazarAPI.TRIG_ENGINE_K,			// U32 -- trigger engine id
                    AlazarAPI.TRIG_DISABLE,			    // U32 -- trigger source id for engine K
                    AlazarAPI.TRIGGER_SLOPE_POSITIVE,	// U32 -- trigger slope id
                    128						            // U32 -- trigger level from 0 (-range) to 255 (+range)
                    );
            if (retCode != AlazarAPI.ApiSuccess)
            {
                Console.WriteLine("Error: AlazarSetTriggerOperation failed -- " +
                    AlazarAPI.AlazarErrorToText(retCode));
                return false;
            }

            // TODO: Select external trigger parameters as required

            retCode =
                AlazarAPI.AlazarSetExternalTrigger(
                    boardHandle,		                // HANDLE -- board handle
                    AlazarAPI.DC_COUPLING,			    // U32 -- external trigger coupling id
                    AlazarAPI.ETR_5V					// U32 -- external trigger range id
                    );

            if (retCode != AlazarAPI.ApiSuccess)
            {
                Console.WriteLine("Error: AlazarSetExternalTrigger failed -- " +
                    AlazarAPI.AlazarErrorToText(retCode));
                return false;
            }

            // TODO: Set trigger delay as required. 

            double triggerDelay_sec = 0;
            UInt32 triggerDelay_samples = (UInt32)(triggerDelay_sec * samplesPerSec + 0.5);
            retCode =
                AlazarAPI.AlazarSetTriggerDelay(
                    boardHandle,                        // HANDLE -- board handle
                    triggerDelay_samples                // U32 -- trigger delay in samples
                    );
            if (retCode != AlazarAPI.ApiSuccess)
            {
                Console.WriteLine("Error: AlazarSetTriggerDelay failed -- " +
                    AlazarAPI.AlazarErrorToText(retCode));
                return false;
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

            double triggerTimeout_sec = 0;
            UInt32 triggerTimeout_clocks = (UInt32)(triggerTimeout_sec / 10E-6 + 0.5);

            retCode =
                AlazarAPI.AlazarSetTriggerTimeOut(
                    boardHandle,		            // HANDLE -- board handle
                    triggerTimeout_clocks	        // U32 -- timeout_sec / 10.e-6 (0 means wait forever)
                    );
            if (retCode != AlazarAPI.ApiSuccess)
            {
                Console.WriteLine("Error: AlazarSetTriggerTimeOut failed -- " +
                    AlazarAPI.AlazarErrorToText(retCode));
                return false;
            }

            // TODO: Configure AUX I/O connector as required

            retCode =
                AlazarAPI.AlazarConfigureAuxIO(
                    boardHandle,			        // HANDLE -- board handle
                    AlazarAPI.AUX_OUT_TRIGGER,		// U32 -- mode
                    0						        // U32 -- parameter
                    );
            if (retCode != AlazarAPI.ApiSuccess)
            {
                Console.WriteLine("Error: AlazarConfigureAuxIO failed -- " +
                    AlazarAPI.AlazarErrorToText(retCode));
                return false;
            }

            return true;
        }

        //----------------------------------------------------------------------------
        //
        // Function    :  Acquire data
        //
        // Description :  Acquire data from board, optionally saving data to file.
        //
        //----------------------------------------------------------------------------

        static public unsafe bool AcquireData(IntPtr boardHandle)
        {
            // TODO: Select the number of pre-trigger samples per record 

            UInt32 preTriggerSamples = 4096;

            // TODO: Select the number of post-trigger samples per record 

            UInt32 postTriggerSamples = 4096;

            // TODO: Specify the number of records to acquire to on-board memory

            UInt32 recordsPerCapture = 100;

            // TODO: Select the amount of time, in seconds, to wait for the 
            //       acquisiton to complete to on-board memory.

            int timeout_ms = 10000;

            // TODO: Select which channels to capture (A, B, or both)

            UInt32 channelMask = AlazarAPI.CHANNEL_A | AlazarAPI.CHANNEL_B;

            // TODO: Select if you wish to save the sample data to a file

            bool saveData = false;

            // Calculate the number of enabled channels from the channel mask 

            UInt32 channelCount = 0;
            switch (channelMask)
            {
                case AlazarAPI.CHANNEL_A:
                case AlazarAPI.CHANNEL_B:
                    channelCount = 1;
                    break;
                case AlazarAPI.CHANNEL_A | AlazarAPI.CHANNEL_B:
                    channelCount = 2;
                    break;
                default:
                    Console.WriteLine("Error: Invalid channel mask -- {0}", channelMask);
                    return false;
            }

            // Get the sample size in bits, and the on-board memory size in samples per channel

            Byte bitsPerSample;
            UInt32 maxSamplesPerChannel;
            UInt32 retCode = AlazarAPI.AlazarGetChannelInfo(boardHandle, &maxSamplesPerChannel, &bitsPerSample);
            if (retCode != AlazarAPI.ApiSuccess)
            {
                Console.WriteLine("Error: AlazarGetChannelInfo failed -- " +
                    AlazarAPI.AlazarErrorToText(retCode));
                return false;
            }

            // Calculate the size of each DMA buffer in bytes

            UInt32 bytesPerSample = ((UInt32)bitsPerSample + 7) / 8;
            UInt32 samplesPerRecord = preTriggerSamples + postTriggerSamples;
            UInt32 bytesPerRecord = bytesPerSample * samplesPerRecord;

            // Calculate the size of a record buffer in bytes
            // Note that the buffer must be at least 16 samples larger than the transfer size

            UInt32 bytesPerBuffer = bytesPerSample * (samplesPerRecord + 16);

            // Configure the record size 

            retCode =
                AlazarAPI.AlazarSetRecordSize(
                    boardHandle,			// HANDLE -- board handle
                    preTriggerSamples,		// U32 -- pre-trigger samples
                    postTriggerSamples		// U32 -- post-trigger samples
                    );
            if (retCode != AlazarAPI.ApiSuccess)
            {
                Console.WriteLine("Error: AlazarSetRecordSize failed -- " + AlazarAPI.AlazarErrorToText(retCode));
                return false;
            }

            // Configure the number of records in the acquisition

            retCode = AlazarAPI.AlazarSetRecordCount(boardHandle, recordsPerCapture);
            if (retCode != AlazarAPI.ApiSuccess)
            {
                Console.WriteLine("Error: AlazarSetRecordCount failed -- " + AlazarAPI.AlazarErrorToText(retCode));
                return false;
            }

            // Arm the board to wait for a trigger event to begin the acquisition 

            retCode = AlazarAPI.AlazarStartCapture(boardHandle);
            if (retCode != AlazarAPI.ApiSuccess)
            {
                Console.WriteLine("Error: AlazarStartCapture failed -- " + AlazarAPI.AlazarErrorToText(retCode));
                return false;
            }

            // Wait for the board to capture all records to on-board memory

            Console.WriteLine("Capturing {0} record ... press any key to abort",
                recordsPerCapture);

            int startTickCount = System.Environment.TickCount;
            int timeoutTickCount = startTickCount + timeout_ms;

            bool success = true;
            while (AlazarAPI.AlazarBusy(boardHandle) != 0 && success == true)
            {
                if (System.Environment.TickCount > timeoutTickCount)
                {
                    Console.WriteLine("Error: Capture timeout after {0} ms", timeout_ms);
                    success = false;
                }
                else if (System.Console.KeyAvailable == true)
                {
                    Console.WriteLine("Acquisition aborted");
                    success = false;
                }
                else
                {
                    System.Threading.Thread.Sleep(10);
                }
            }

            if (!success)
            {
                // Abort the acquisition
                retCode = AlazarAPI.AlazarAbortCapture(boardHandle);
                if (retCode != AlazarAPI.ApiSuccess)
                {
                    Console.WriteLine("Error: AlazarAbortCapture failed -- " + AlazarAPI.AlazarErrorToText(retCode));
                }
                return false;
            }

            // The board captured all records to on-board memory

            double captureTime_sec = ((double)(System.Environment.TickCount - startTickCount)) / 1000;

            double recordsPerSec;
            if (captureTime_sec > 0)
                recordsPerSec = recordsPerCapture / captureTime_sec;
            else
                recordsPerSec = 0;

            Console.WriteLine("Captured {0} records in {1:N3} sec ({2:N3} records / sec)",
                recordsPerCapture, captureTime_sec, recordsPerSec);

            StreamWriter file = null;
            startTickCount = System.Environment.TickCount;
            UInt64 bytesTransferred = 0;

            try
            {
                // Create a data file if required

                if (saveData)
                {
                    file = new StreamWriter(@"data.txt");
                }

                // Allocate memory for one record

                byte[] buffer = new byte[bytesPerBuffer];
                fixed (byte* pBuffer = buffer)
                {
                    // Transfer the records from on-board memory to our buffer

                    Console.WriteLine("Transferring {0} records ... press any key to cancel", recordsPerCapture);

                    UInt32 record;
                    for (record = 0; record < recordsPerCapture; record++)
                    {
                        for (int channel = 0; channel < channelCount; channel++)
                        {
                            // Find the current channel Id

                            UInt32 channelId;
                            if (channelCount == 1)
                            {
                                if ((channelMask & AlazarAPI.CHANNEL_A) != 0)
                                    channelId = AlazarAPI.CHANNEL_A;
                                else
                                    channelId = AlazarAPI.CHANNEL_B;
                            }
                            else
                            {
                                if (channel == 0)
                                    channelId = AlazarAPI.CHANNEL_A;
                                else
                                    channelId = AlazarAPI.CHANNEL_B;
                            }

                            // Transfer one full record from on-board memory to our buffer

                            retCode =
                                AlazarAPI.AlazarRead(
                                    boardHandle,				// HANDLE -- board handle
                                    channelId,					// U32 -- channel Id
                                    pBuffer,				    // void* -- buffer
                                    (int)bytesPerSample,		// int -- bytes per sample
                                    (int)record + 1,			// long -- record (1 indexed)
                                    -((int)preTriggerSamples),	// long -- offset from trigger in samples
                                    samplesPerRecord			// U32 -- samples to transfer
                                    );
                            if (retCode != AlazarAPI.ApiSuccess)
                            {
                                throw new System.Exception("Error: AlazarRead record  -- " +
                                    AlazarAPI.AlazarErrorToText(retCode));
                            }

                            bytesTransferred += bytesPerRecord;

                            // TODO: Process record here.
                            // 
                            // Samples values are arranged contiguously in the buffer, with
                            // 8-bit sample codes in each 8-bit sample value. 
                            //
                            // Sample codes are unsigned by default so that:
                            // - a sample code of 0x00 represents a negative full scale input signal;
                            // - a sample code of 0x80 represents a ~0V signal;
                            // - a sample code of 0xFF represents a positive full scale input signal.

                            if (saveData)
                            {
                                UInt32 inputRangeId;
                                if (channelId == AlazarAPI.CHANNEL_A)
                                    inputRangeId = inputRangeIdChA;
                                else
                                    inputRangeId = inputRangeIdChB;

                                double inputRange_volts;
                                if (!InputRangeIdToVolts(inputRangeId, out inputRange_volts))
                                {
                                    throw new System.Exception("Error: Invalid input range -- " + inputRangeId);
                                }					

                                double codeZero = (1 << (bitsPerSample - 1)) - 0.5;
                                double codeRange = (1 << (bitsPerSample - 1)) - 0.5;

                                // Find current channel name from channel Id

                                char channelName;
                                if (channelId == AlazarAPI.CHANNEL_A)
                                    channelName = 'A';
                                else
                                    channelName = 'B';

                                // Write sample values file
                                file.WriteLine("--> CH{0} record {1} begin", channelName, record + 1);
                                file.WriteLine("");

                                for (UInt32 sample = 0; sample < samplesPerRecord; sample++)
                                {
                                    // Get sample code from buffer
                                    byte sampleCode = buffer[sample];

                                    // Convert sample code to volts
                                    double sampleVolts = inputRange_volts * ((double) (sampleCode - codeZero) / codeRange);

                                    // write the value to file
                                    file.WriteLine("{0} \t{1:G4}", sample, sampleVolts);
                                }

                                file.WriteLine("<-- CH{0} record {1} end", channelName, record + 1);
                                file.WriteLine("");
                            }
                        }

                        // If a key was pressed, then stop processing records.

                        if (Console.KeyAvailable == true)
                        {
                            throw new System.Exception("Error: Transfer aborted");
                        }
                    }
                }
            }
            catch (Exception exception)
            {
                Console.WriteLine(exception.ToString());
                success = false;
            }
            finally
            {
                // Close the data file
                if (file != null)
                    file.Close();
            }

            // Display results

            double transferTime_sec = ((double)(System.Environment.TickCount - startTickCount)) / 1000;

            double bytesPerSec;
            if (transferTime_sec > 0)
                bytesPerSec = bytesTransferred / transferTime_sec;
            else
                bytesPerSec = 0;

            Console.WriteLine("Transferred {0} bytes ({1:G4} bytes per sec)", bytesTransferred, bytesPerSec);

            return success;
        }

        //----------------------------------------------------------------------------
        //
        // Function    :  InputRangeIdToVolts
        //
        // Description :  Convert input range identifier to volts
        //
        //----------------------------------------------------------------------------

        static public bool InputRangeIdToVolts(UInt32 inputRangeId, out double inputRangeVolts)
        {
	        switch (inputRangeId)
	        {
	        case AlazarAPI.INPUT_RANGE_PM_20_MV:
		        inputRangeVolts = 20.0e-3;
		        break;
	        case AlazarAPI.INPUT_RANGE_PM_40_MV:
		        inputRangeVolts = 40.0e-3;
		        break;
	        case AlazarAPI.INPUT_RANGE_PM_50_MV:
		        inputRangeVolts = 50.0e-3;
		        break;
	        case AlazarAPI.INPUT_RANGE_PM_80_MV:
		        inputRangeVolts = 80.0e-3;
		        break;
	        case AlazarAPI.INPUT_RANGE_PM_100_MV:
		        inputRangeVolts = 100.0e-3;
		        break;
	        case AlazarAPI.INPUT_RANGE_PM_200_MV:
		        inputRangeVolts = 200.0e-3;
		        break;
	        case AlazarAPI.INPUT_RANGE_PM_400_MV:
		        inputRangeVolts = 400.0e-3;
		        break;
	        case AlazarAPI.INPUT_RANGE_PM_500_MV:
		        inputRangeVolts = 500.0e-3;
		        break;
	        case AlazarAPI.INPUT_RANGE_PM_800_MV:
		        inputRangeVolts = 800.0e-3;
		        break;
	        case AlazarAPI.INPUT_RANGE_PM_1_V:
		        inputRangeVolts = 1.0;
		        break;
	        case AlazarAPI.INPUT_RANGE_PM_2_V:
		        inputRangeVolts = 2.0;
		        break;
	        case AlazarAPI.INPUT_RANGE_PM_4_V:
		        inputRangeVolts = 4.0;
		        break;
	        case AlazarAPI.INPUT_RANGE_PM_5_V:
		        inputRangeVolts = 5.0;
		        break;
	        case AlazarAPI.INPUT_RANGE_PM_8_V:
		        inputRangeVolts = 8.0;
		        break;
	        case AlazarAPI.INPUT_RANGE_PM_10_V:
		        inputRangeVolts = 10.0;
		        break;
	        case AlazarAPI.INPUT_RANGE_PM_20_V:
		        inputRangeVolts = 20.0;
		        break;
	        case AlazarAPI.INPUT_RANGE_PM_40_V:
		        inputRangeVolts = 40.0;
		        break;
	        case AlazarAPI.INPUT_RANGE_PM_16_V:
		        inputRangeVolts = 16.0;
		        break;
	        default:
                inputRangeVolts = -1.0;
		        Console.WriteLine("Error: Invalid input range {0}", inputRangeId);
		        return false;
	        }

	        return true;
        }
    }
}

