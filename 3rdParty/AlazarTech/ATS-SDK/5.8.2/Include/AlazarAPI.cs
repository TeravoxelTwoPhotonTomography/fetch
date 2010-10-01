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

// -------------------------------------------------------------------------
// Title:   AlazarAPI.cs
// Date:    2010/03/30
// Version: 5.7.8
// --------------------------------------------------------------------------

using System;
using System.Runtime.InteropServices;

namespace AlazarTech
{
    class AlazarAPI
    {
        #region - Constants -------------------------------------------------

        #region - Return Codes ----------------------------------------
        public const UInt32 ApiSuccess = 512;
        public const UInt32 ApiFailed = 513;
        public const UInt32 ApiAccessDenied = 514;
        public const UInt32 ApiDmaChannelUnavailable = 515;
        public const UInt32 ApiDmaChannelInvalid = 516;
        public const UInt32 ApiDmaChannelTypeError = 517;
        public const UInt32 ApiDmaInProgress = 518;
        public const UInt32 ApiDmaDone = 519;
        public const UInt32 ApiDmaPaused = 520;
        public const UInt32 ApiDmaNotPaused = 521;
        public const UInt32 ApiDmaCommandInvalid = 522;
        public const UInt32 ApiDmaManReady = 523;
        public const UInt32 ApiDmaManNotReady = 524;
        public const UInt32 ApiDmaInvalidChannelPriority = 525;
        public const UInt32 ApiDmaManCorrupted = 526;
        public const UInt32 ApiDmaInvalidElementIndex = 527;
        public const UInt32 ApiDmaNoMoreElements = 528;
        public const UInt32 ApiDmaSglInvalid = 529;
        public const UInt32 ApiDmaSglQueueFull = 530;
        public const UInt32 ApiNullParam = 531;
        public const UInt32 ApiInvalidBusIndex = 532;
        public const UInt32 ApiUnsupportedFunction = 533;
        public const UInt32 ApiInvalidPciSpace = 534;
        public const UInt32 ApiInvalidIopSpace = 535;
        public const UInt32 ApiInvalidSize = 536;
        public const UInt32 ApiInvalidAddress = 537;
        public const UInt32 ApiInvalidAccessType = 538;
        public const UInt32 ApiInvalidIndex = 539;
        public const UInt32 ApiMuNotReady = 540;
        public const UInt32 ApiMuFifoEmpty = 541;
        public const UInt32 ApiMuFifoFull = 542;
        public const UInt32 ApiInvalidRegister = 543;
        public const UInt32 ApiDoorbellClearFailed = 544;
        public const UInt32 ApiInvalidUserPin = 545;
        public const UInt32 ApiInvalidUserState = 546;
        public const UInt32 ApiEepromNotPresent = 547;
        public const UInt32 ApiEepromTypeNotSupported = 548;
        public const UInt32 ApiEepromBlank = 549;
        public const UInt32 ApiConfigAccessFailed = 550;
        public const UInt32 ApiInvalidDeviceInfo = 551;
        public const UInt32 ApiNoActiveDriver = 552;
        public const UInt32 ApiInsufficientResources = 553;
        public const UInt32 ApiObjectAlreadyAllocated = 554;
        public const UInt32 ApiAlreadyInitialized = 555;
        public const UInt32 ApiNotInitialized = 556;
        public const UInt32 ApiBadConfigRegEndianMode = 557;
        public const UInt32 ApiInvalidPowerState = 558;
        public const UInt32 ApiPowerDown = 559;
        public const UInt32 ApiFlybyNotSupported = 560;
        public const UInt32 ApiNotSupportThisChannel = 561;
        public const UInt32 ApiNoAction = 562;
        public const UInt32 ApiHSNotSupported = 563;
        public const UInt32 ApiVPDNotSupported = 564;
        public const UInt32 ApiVpdNotEnabled = 565;
        public const UInt32 ApiNoMoreCap = 566;
        public const UInt32 ApiInvalidOffset = 567;
        public const UInt32 ApiBadPinDirection = 568;
        public const UInt32 ApiPciTimeout = 569;
        public const UInt32 ApiDmaChannelClosed = 570;
        public const UInt32 ApiDmaChannelError = 571;
        public const UInt32 ApiInvalidHandle = 572;
        public const UInt32 ApiBufferNotReady = 573;
        public const UInt32 ApiInvalidData = 574;
        public const UInt32 ApiDoNothing = 575;
        public const UInt32 ApiDmaSglBuildFailed = 576;
        public const UInt32 ApiPMNotSupported = 577;
        public const UInt32 ApiInvalidDriverVersion = 578;
        public const UInt32 ApiWaitTimeout = 579;
        public const UInt32 ApiWaitCanceled = 580;
        public const UInt32 ApiBufferTooSmall = 581;
        public const UInt32 ApiBufferOverflow = 582;
        public const UInt32 ApiInvalidBuffer = 583;
        public const UInt32 ApiInvalidRecordsPerBuffer = 584;
        public const UInt32 ApiDmaPending = 585;
        public const UInt32 ApiLockAndProbePagesFailed = 586;
        public const UInt32 ApiWaitAbandoned = 587;
        public const UInt32 ApiWaitFailed = 588;
        public const UInt32 ApiTransferComplete = 589;
        public const UInt32 ApiPllNotLocked = 590;
        public const UInt32 ApiNotSupportedInDualChannelMode = 591;
	public const UInt32 ApiFileIoError = 592;
        public const UInt32 ApiLastError = 593;
        #endregion

        #region - Board Types -----------------------------------------
        public const UInt32 ATS_NONE = 0;
        public const UInt32 ATS850 = 1;
        public const UInt32 ATS310 = 2;
        public const UInt32 ATS330 = 3;
        public const UInt32 ATS855 = 4;
        public const UInt32 ATS315 = 5;
        public const UInt32 ATS335 = 6;
        public const UInt32 ATS460 = 7;
        public const UInt32 ATS860 = 8;
        public const UInt32 ATS660 = 9;
        public const UInt32 ATS665 = 10;
        public const UInt32 ATS9462 = 11;
        public const UInt32 ATS9434 = 12;
        public const UInt32 ATS9870 = 13;
        public const UInt32 ATS9350 = 14;
        public const UInt32 ATS_LAST = 15;
        #endregion

        #region - Memory Sizes ----------------------------------------
        public const int MEM8K = 0;
        public const int MEM64K = 1;
        public const int MEM128K = 2;
        public const int MEM256K = 3;
        public const int MEM512K = 4;
        public const int MEM1M = 5;
        public const int MEM2M = 6;
        public const int MEM4M = 7;
        public const int MEM8M = 8;
        public const int MEM16M = 9;
        public const int MEM32M = 10;
        public const int MEM64M = 11;
        public const int MEM128M = 12;
        public const int MEM256M = 13;
        public const int MEM512M = 14;
        public const int MEM1G = 15;
        public const int MEM2G = 16;
        public const int MEM4G = 17;
        public const int MEM8G = 18;
        public const int MEM16G = 19;
        #endregion

        #region - Clock control ---------------------------------------

        // Clock Sources
        public const UInt32 INTERNAL_CLOCK = 0x00000001;
        public const UInt32 EXTERNAL_CLOCK = 0x00000002;
        public const UInt32 FAST_EXTERNAL_CLOCK = 0x00000002;
        public const UInt32 MEDIUM_EXTERNAL_CLOCK = 0x00000003;
        public const UInt32 SLOW_EXTERNAL_CLOCK = 0x00000004;
        public const UInt32 EXTERNAL_CLOCK_AC = 0x00000005;
        public const UInt32 EXTERNAL_CLOCK_DC = 0x00000006;
        public const UInt32 EXTERNAL_CLOCK_10MHz_REF = 0x00000007;
	public const UInt32 INTERNAL_CLOCK_DIV_5 = 0x000000010;

        // Internal Sample Rates
        public const UInt32 SAMPLE_RATE_1KSPS = 0x00000001;
        public const UInt32 SAMPLE_RATE_2KSPS = 0x00000002;
        public const UInt32 SAMPLE_RATE_5KSPS = 0x00000004;
        public const UInt32 SAMPLE_RATE_10KSPS = 0x00000008;
        public const UInt32 SAMPLE_RATE_20KSPS = 0x0000000A;
        public const UInt32 SAMPLE_RATE_50KSPS = 0x0000000C;
        public const UInt32 SAMPLE_RATE_100KSPS = 0x0000000E;
        public const UInt32 SAMPLE_RATE_200KSPS = 0x00000010;
        public const UInt32 SAMPLE_RATE_500KSPS = 0x00000012;
        public const UInt32 SAMPLE_RATE_1MSPS = 0x00000014;
        public const UInt32 SAMPLE_RATE_2MSPS = 0x00000018;
        public const UInt32 SAMPLE_RATE_5MSPS = 0x0000001A;
        public const UInt32 SAMPLE_RATE_10MSPS = 0x0000001C;
        public const UInt32 SAMPLE_RATE_20MSPS = 0x0000001E;
        public const UInt32 SAMPLE_RATE_25MSPS = 0x00000021;
        public const UInt32 SAMPLE_RATE_50MSPS = 0x00000022;
        public const UInt32 SAMPLE_RATE_100MSPS = 0x00000024;
        public const UInt32 SAMPLE_RATE_125MSPS = 0x00000025;
        public const UInt32 SAMPLE_RATE_200MSPS = 0x00000028;
        public const UInt32 SAMPLE_RATE_250MSPS = 0x0000002B;
        public const UInt32 SAMPLE_RATE_500MSPS = 0x00000030;
        public const UInt32 SAMPLE_RATE_1GSPS = 0x00000035;
        public const UInt32 SAMPLE_RATE_2GSPS = 0x0000003A;
        public const UInt32 SAMPLE_RATE_USER_DEF = 0x00000040;
        public const UInt32 PLL_10MHZ_REF_100MSPS_BASE = 0x05F5E100;

        // Clock Edges
        public const UInt32 CLOCK_EDGE_RISING = 0x00000000;
        public const UInt32 CLOCK_EDGE_FALLING = 0x00000001;

        // Decimation
        public const UInt32 DECIMATE_BY_8 = 0x00000008;
        public const UInt32 DECIMATE_BY_64 = 0x00000040;
        #endregion

        #region - Input Control ---------------------------------------

        // Input Channels
        public const UInt32 CHANNEL_ALL = 0x00000000;
        public const UInt32 CHANNEL_A = 0x00000001;
        public const UInt32 CHANNEL_B = 0x00000002;
        public const UInt32 CHANNEL_C = 0x00000003;
        public const UInt32 CHANNEL_D = 0x00000004;
        public const UInt32 CHANNEL_E = 0x00000005;
        public const UInt32 CHANNEL_F = 0x00000006;
        public const UInt32 CHANNEL_G = 0x00000007;
        public const UInt32 CHANNEL_H = 0x00000008;

        // Input Ranges
        public const UInt32 INPUT_RANGE_PM_20_MV = 0x00000001;
        public const UInt32 INPUT_RANGE_PM_40_MV = 0x00000002;
        public const UInt32 INPUT_RANGE_PM_50_MV = 0x00000003;
        public const UInt32 INPUT_RANGE_PM_80_MV = 0x00000004;
        public const UInt32 INPUT_RANGE_PM_100_MV = 0x00000005;
        public const UInt32 INPUT_RANGE_PM_200_MV = 0x00000006;
        public const UInt32 INPUT_RANGE_PM_400_MV = 0x00000007;
        public const UInt32 INPUT_RANGE_PM_500_MV = 0x00000008;
        public const UInt32 INPUT_RANGE_PM_800_MV = 0x00000009;
        public const UInt32 INPUT_RANGE_PM_1_V = 0x0000000A;
        public const UInt32 INPUT_RANGE_PM_2_V = 0x0000000B;
        public const UInt32 INPUT_RANGE_PM_4_V = 0x0000000C;
        public const UInt32 INPUT_RANGE_PM_5_V = 0x0000000D;
        public const UInt32 INPUT_RANGE_PM_8_V = 0x0000000E;
        public const UInt32 INPUT_RANGE_PM_10_V = 0x0000000F;
        public const UInt32 INPUT_RANGE_PM_20_V = 0x00000010;
        public const UInt32 INPUT_RANGE_PM_40_V = 0x00000011;
        public const UInt32 INPUT_RANGE_PM_16_V = 0x00000012;
        public const UInt32 INPUT_RANGE_HIFI = 0x00000020;

        // Input Impedances
        public const UInt32 IMPEDANCE_1M_OHM = 0x00000001;
        public const UInt32 IMPEDANCE_50_OHM = 0x00000002;
        public const UInt32 IMPEDANCE_75_OHM = 0x00000004;
        public const UInt32 IMPEDANCE_300_OHM = 0x00000008;

        // Input Couplings
        public const UInt32 AC_COUPLING = 0x00000001;
        public const UInt32 DC_COUPLING = 0x00000002;

        #endregion

        #region - Trigger Control  ------------------------------------

        // Trigger Engines
        public const UInt32 TRIG_ENGINE_J = 0x00000000;
        public const UInt32 TRIG_ENGINE_K = 0x00000001;

        // Trigger Engine Operations
        public const UInt32 TRIG_ENGINE_OP_J = 0x00000000;
        public const UInt32 TRIG_ENGINE_OP_K = 0x00000001;
        public const UInt32 TRIG_ENGINE_OP_J_OR_K = 0x00000002;
        public const UInt32 TRIG_ENGINE_OP_J_AND_K = 0x00000003;
        public const UInt32 TRIG_ENGINE_OP_J_XOR_K = 0x00000004;
        public const UInt32 TRIG_ENGINE_OP_J_AND_NOT_K = 0x00000005;
        public const UInt32 TRIG_ENGINE_OP_NOT_J_AND_K = 0x00000006;

        // Trigger Engine Sources
        public const UInt32 TRIG_CHAN_A = 0x00000000;
        public const UInt32 TRIG_CHAN_B = 0x00000001;
        public const UInt32 TRIG_EXTERNAL = 0x00000002;
        public const UInt32 TRIG_DISABLE = 0x00000003;

        // Trigger Slopes
        public const UInt32 TRIGGER_SLOPE_POSITIVE = 0x00000001;
        public const UInt32 TRIGGER_SLOPE_NEGATIVE = 0x00000002;

        // External Trigger Ranges
        public const UInt32 ETR_DIV5 = 0x00000000;
        public const UInt32 ETR_X1 = 0x00000001;
        public const UInt32 ETR_5V = 0x00000000;
        public const UInt32 ETR_1V = 0x00000001;

        #endregion

        #region - AUX_IO and LED Control ------------------------------

        // Inputs
        public const UInt32 AUX_OUT_TRIGGER = 0;
        public const UInt32 AUX_OUT_PACER = 2;
        public const UInt32 AUX_OUT_BUSY = 4;
        public const UInt32 AUX_OUT_CLOCK = 6;
        public const UInt32 AUX_OUT_RESERVED = 8;
        public const UInt32 AUX_OUT_CAPTURE_ALMOST_DONE = 10;
        public const UInt32 AUX_OUT_AUXILIARY = 12;
        public const UInt32 AUX_OUT_SERIAL_DATA = 14;
	public const UInt32 AUX_OUT_TRIGGER_ENABLE = 20;

        // Outputs
        public const UInt32 AUX_IN_TRIGGER_ENABLE = 1;
        public const UInt32 AUX_IN_DIGITAL_TRIGGER = 3;
        public const UInt32 AUX_IN_GATE = 5;
        public const UInt32 AUX_IN_CAPTURE_ON_DEMAND = 7;
        public const UInt32 AUX_IN_RESET_TIMESTAMP = 9;
        public const UInt32 AUX_IN_SLOW_EXTERNAL_CLOCK = 11;
        public const UInt32 AUX_INPUT_AUXILIARY = 13;
        public const UInt32 AUX_INPUT_SERIAL_DATA = 15;

        // LED states
        public const UInt32 LED_OFF = 0x00000000;
        public const UInt32 LED_ON = 0x00000001;

        #endregion

        #region - Set / Get Parameters --------------------------------

        public const UInt32 NUMBER_OF_RECORDS = 0x10000001;
        public const UInt32 PRETRIGGER_AMOUNT = 0x10000002;
        public const UInt32 RECORD_LENGTH = 0x10000003;
        public const UInt32 TRIGGER_ENGINE = 0x10000004;
        public const UInt32 TRIGGER_DELAY = 0x10000005;
        public const UInt32 TRIGGER_TIMEOUT = 0x10000006;
        public const UInt32 SAMPLE_RATE = 0x10000007;
        public const UInt32 CONFIGURATION_MODE = 0x10000008; //Independent, Master/Slave, Last Slave
        public const UInt32 DATA_WIDTH = 0x10000009; //8,16,32 bits - Digital IO boards
        public const UInt32 SAMPLE_SIZE = DATA_WIDTH; //8,12,16 - Analog Input boards
        public const UInt32 AUTO_CALIBRATE = 0x1000000A;
        public const UInt32 TRIGGER_XXXXX = 0x1000000B;
        public const UInt32 CLOCK_SOURCE = 0x1000000C;
        public const UInt32 CLOCK_SLOPE = 0x1000000D;
        public const UInt32 IMPEDANCE = 0x1000000E;
        public const UInt32 INPUT_RANGE = 0x1000000F;
        public const UInt32 COUPLING = 0x10000010;
        public const UInt32 MAX_TIMEOUTS_ALLOWED = 0x10000011;
        public const UInt32 ATS_OPERATING_MODE = 0x10000012; //Single, Dual, Quad etc...
        public const UInt32 CLOCK_DECIMATION_EXTERNAL = 0x10000013;
        public const UInt32 LED_CONTROL = 0x10000014;
        public const UInt32 ATTENUATOR_RELAY = 0x10000018;
        public const UInt32 EXT_TRIGGER_COUPLING = 0x1000001A;
        public const UInt32 EXT_TRIGGER_ATTENUATOR_RELAY = 0x1000001C;
        public const UInt32 TRIGGER_ENGINE_SOURCE = 0x1000001E;
        public const UInt32 TRIGGER_ENGINE_SLOPE = 0x10000020;
        public const UInt32 SEND_DAC_VALUE = 0x10000021;
        public const UInt32 SLEEP_DEVICE = 0x10000022;
        public const UInt32 GET_DAC_VALUE = 0x10000023;
        public const UInt32 GET_SERIAL_NUMBER = 0x10000024;
        public const UInt32 GET_FIRST_CAL_DATE = 0x10000025;
        public const UInt32 GET_LATEST_CAL_DATE = 0x10000026;
        public const UInt32 GET_LATEST_TEST_DATE = 0x10000027;
        public const UInt32 GET_LATEST_CAL_DATE_MONTH = 0x1000002D;
        public const UInt32 GET_LATEST_CAL_DATE_DAY = 0x1000002E;
        public const UInt32 GET_LATEST_CAL_DATE_YEAR = 0x1000002F;
        public const UInt32 GET_PCIE_LINK_SPEED = 0x10000030;
        public const UInt32 GET_PCIE_LINK_WIDTH = 0x10000031;
        public const UInt32 SETGET_ASYNC_BUFFCOUNT = 0x10000040;
        public const UInt32 SET_DATA_FORMAT = 0x10000041;
        public const UInt32 GET_DATA_FORMAT = 0x10000042;
        public const UInt32 DATA_FORMAT_UNSIGNED = 0;
        public const UInt32 DATA_FORMAT_SIGNED = 1;
        public const UInt32 SET_SINGLE_CHANNEL_MODE = 0x10000043;

        public const UInt32 MEMORY_SIZE = 0x1000002A;
        public const UInt32 BOARD_TYPE = 0x1000002B;
        public const UInt32 ASOPC_TYPE = 0x1000002C;
        public const UInt32 GET_BOARD_OPTIONS_LOW = 0x10000037;
        public const UInt32 GET_BOARD_OPTIONS_HIGH = 0x10000038;
        public const UInt32 OPTION_STREAMING_DMA = (1U << 0);
        public const UInt32 OPTION_AVERAGE_INPUT = (1U << 1);
        public const UInt32 OPTION_EXTERNAL_CLOCK = (1U << 1);
        public const UInt32 OPTION_DUAL_PORT_MEMORY = (1U << 2);
        public const UInt32 OPTION_180MHZ_OSCILLATOR = (1U << 3);
        public const UInt32 OPTION_LVTTL_EXT_CLOCK = (1U << 4);
        public const UInt32 OPTION_OEM_FPGA = (1U << 47);

        public const UInt32 TRANSFER_OFFET = 0x10000030;
        public const UInt32 TRANSFER_LENGTH = 0x10000031;
        public const UInt32 TRANSFER_RECORD_OFFSET = 0x10000032;
        public const UInt32 TRANSFER_NUM_OF_RECORDS = 0x10000033;
        public const UInt32 TRANSFER_MAPPING_RATIO = 0x10000034;
        public const UInt32 TRIGGER_ADDRESS_AND_TIMESTAMP = 0x10000035;
        public const UInt32 MASTER_SLAVE_INDEPENDENT = 0x10000036;

        public const UInt32 TRIGGERED = 0x10000040;
        public const UInt32 BUSY = 0x10000041;
        public const UInt32 WHO_TRIGGERED = 0x10000042;
	public const UInt32 GET_ASYNC_BUFFERS_PENDING = 0x10000050;
	public const UInt32 GET_ASYNC_BUFFERS_PENDING_FULL = 0x10000051;
	public const UInt32 GET_ASYNC_BUFFERS_PENDING_EMPTY = 0x10000052;
	public const UInt32 ACF_SAMPLES_PER_RECORD = 0x10000060;
	public const UInt32 ACF_RECORDS_TO_AVERAGE = 0x10000061;

        // Master/Slave Configuration
        public const UInt32 BOARD_IS_INDEPENDENT = 0x00000000;
        public const UInt32 BOARD_IS_MASTER = 0x00000001;
        public const UInt32 BOARD_IS_SLAVE = 0x00000002;
        public const UInt32 BOARD_IS_LAST_SLAVE = 0x00000003;

        // Attenuator Relay
        public const UInt32 AR_X1 = 0x00000000;
        public const UInt32 AR_DIV40 = 0x00000001;

        // Device Sleep state
        public const UInt32 POWER_OFF = 0x00000000;
        public const UInt32 POWER_ON = 0x00000001;

        // Software Events control
        public const UInt32 SW_EVENTS_OFF = 0x00000000;
        public const UInt32 SW_EVENTS_ON = 0x00000001;

        // TimeStamp Value Reset Control
        public const UInt32 TIMESTAMP_RESET_FIRSTTIME_ONLY = 0x00000000;
        public const UInt32 TIMESTAMP_RESET_ALWAYS = 0x00000001;

        // DAC Names used by API AlazarDACSettingAdjust 
        public const UInt32 ATS460_DAC_A_GAIN = 0x00000001;
        public const UInt32 ATS460_DAC_A_OFFSET = 0x00000002;
        public const UInt32 ATS460_DAC_A_POSITION = 0x00000003;
        public const UInt32 ATS460_DAC_B_GAIN = 0x00000009;
        public const UInt32 ATS460_DAC_B_OFFSET = 0x0000000A;
        public const UInt32 ATS460_DAC_B_POSITION = 0x0000000B;

        // Error return values
        public const UInt32 SETDAC_INVALID_SETGET = 660;
        public const UInt32 SETDAC_INVALID_CHANNEL = 661;
        public const UInt32 SETDAC_INVALID_DACNAME = 662;
        public const UInt32 SETDAC_INVALID_COUPLING = 663;
        public const UInt32 SETDAC_INVALID_RANGE = 664;
        public const UInt32 SETDAC_INVALID_IMPEDANCE = 665;
        public const UInt32 SETDAC_BAD_GET_PTR = 667;
        public const UInt32 SETDAC_INVALID_BOARDTYPE = 668;

        // Constants to be used in the Application when dealing with Custom FPGAs
        public const UInt32 FPGA_GETFIRST = 0xFFFFFFFF;
        public const UInt32 FPGA_GETNEXT = 0xFFFFFFFE;
        public const UInt32 FPGA_GETLAST = 0xFFFFFFFC;

        // Global API Functions
        public const int KINDEPENDENT = 0;
        public const int KSLAVE = 1;
        public const int KMASTER = 2;
        public const int KLASTSLAVE = 3;
  
        #endregion

        #region - AutoDMA Control -------------------------------------

        // AutoDMA flags 
        public const UInt32 ADMA_EXTERNAL_STARTCAPTURE = 0x00000001;
        public const UInt32 ADMA_ENABLE_RECORD_HEADERS = 0x00000008;
        public const UInt32 ADMA_SINGLE_DMA_CHANNEL = 0x00000010;
        public const UInt32 ADMA_ALLOC_BUFFERS = 0x00000020;
        public const UInt32 ADMA_TRADITIONAL_MODE = 0x00000000;
        public const UInt32 ADMA_CONTINUOUS_MODE = 0x00000100;
        public const UInt32 ADMA_NPT = 0x00000200;
        public const UInt32 ADMA_TRIGGERED_STREAMING = 0x00000400;
        public const UInt32 ADMA_FIFO_ONLY_STREAMING = 0x00000800;
        public const UInt32 ADMA_INTERLEAVE_SAMPLES = 0x00001000;
        public const UInt32 ADMA_GET_PROCESSED_DATA = 0x00002000;
        public const UInt32 ADMA_STREAM_TO_DISK = 0x00004000;

        // AutoDMA header constants
        public const UInt32 ADMA_CLOCKSOURCE = 0x00000001;
        public const UInt32 ADMA_CLOCKEDGE = 0x00000002;
        public const UInt32 ADMA_SAMPLERATE = 0x00000003;
        public const UInt32 ADMA_INPUTRANGE = 0x00000004;
        public const UInt32 ADMA_INPUTCOUPLING = 0x00000005;
        public const UInt32 ADMA_IMPUTIMPEDENCE = 0x00000006;
        public const UInt32 ADMA_EXTTRIGGERED = 0x00000007;
        public const UInt32 ADMA_CHA_TRIGGERED = 0x00000008;
        public const UInt32 ADMA_CHB_TRIGGERED = 0x00000009;
        public const UInt32 ADMA_TIMEOUT = 0x0000000A;
        public const UInt32 ADMA_THISCHANTRIGGERED = 0x0000000B;
        public const UInt32 ADMA_SERIALNUMBER = 0x0000000C;
        public const UInt32 ADMA_SYSTEMNUMBER = 0x0000000D;
        public const UInt32 ADMA_BOARDNUMBER = 0x0000000E;
        public const UInt32 ADMA_WHICHCHANNEL = 0x0000000F;
        public const UInt32 ADMA_SAMPLERESOLUTION = 0x00000010;
        public const UInt32 ADMA_DATAFORMAT = 0x00000011;

        // _HEADER0 - UInt32 type with the following format
        //  SerialNumber size 18 bits
        //  SystemNumber size 4 bits
        //  WhichChannel size 1 bits
        //  BoardNumber size 4 bits
        //  SampleResolution size 3 bits
        //  DataFormat size 2 bits
        //  struct _HEADER0 { public UInt32 u;}

        // _HEADER1 - UInt32 type with the following format
        //  RecordNumber size 24 bits
        //  BoardType size 8 bits
        //  struct _HEADER1 { public UInt32 u;}

        // _HEADER2 - UInt32 type with the following format
        //  TimeStampLowPart all 32 bits
        //  struct _HEADER2 { public UInt32 u;}


        // _HEADER3 - UInt32 type with the following format
        //  TimeStampHighPart size 8 bits
        //  ClockSource size 2 bits
        //  ClockEdge size 1 bits
        //  SampleRate size 7 bits
        //  InputRange size 5 bits
        //  InputCoupling size 2 bits
        //  InputImpedence size 2 bits
        //  ExternalTriggered size 1 bits
        //  ChannelBTriggered size 1 bits
        //  ChannelATriggered size 1 bits
        //  TimeOutOccurred size 1 bits
        //  ThisChannelTriggered size 1 bits
        //  public struct _HEADER3 { public UInt32 u;}

        //  struct ALAZAR_HEADER {
        //	  public _HEADER0 hdr0;
        //	  public _HEADER1 hdr1;
        //	  public _HEADER2 hdr2;
        //	  public _HEADER3 hdr3;
        //  }

        public enum AUTODMA_STATUS
        {
            ADMA_Completed = 0,
            ADMA_Success = 0,
            ADMA_Buffer1Invalid,
            ADMA_Buffer2Invalid,
            ADMA_BoardHandleInvalid,
            ADMA_InternalBuffer1Invalid,
            ADMA_InternalBuffer2Invalid,
            ADMA_OverFlow,
            ADMA_InvalidChannel,
            ADMA_DMAInProgress,
            ADMA_UseHeaderNotSet,
            ADMA_HeaderNotValid,
            ADMA_InvalidRecsPerBuffer,
            ADMA_InvalidTransferOffset
        }

        //public const UInt32 ADMA_Success = AUTODMA_STATUS.ADMA_Completed;

        #endregion

        #endregion

        #region - Functions -------------------------------------------------

        #region - Status and Information ------------------------------

        [DllImport("ATSApi.dll")]
        public static extern UInt32 AlazarBoardsFound();

        [DllImport("ATSApi.dll")]
        public static extern UInt32 AlazarGetBoardKind(IntPtr h);

        [DllImport("ATSApi.dll")]
        public static extern unsafe UInt32 AlazarGetCPLDVersion(IntPtr h, byte* Major, byte* Minor);

        [DllImport("ATSApi.dll")]
        public static extern unsafe UInt32 AlazarGetChannelInfo(IntPtr h, UInt32* MemSize, byte* SampleSize);

        [DllImport("ATSApi.dll")]
        public static extern unsafe UInt32 AlazarGetSDKVersion(byte* Major, byte* Minor, byte* Revision);

        [DllImport("ATSApi.dll")]
        public static extern unsafe UInt32 AlazarGetDriverVersion(byte* Major, byte* Minor, byte* Revision);

        [DllImport("ATSApi.dll")]
        public static extern IntPtr AlazarGetSystemHandle(UInt32 sid);

        [DllImport("ATSApi.dll")]
        public static extern UInt32 AlazarNumOfSystems();

        [DllImport("ATSApi.dll")]
        public static extern UInt32 AlazarBoardsInSystemBySystemID(UInt32 sid);

        [DllImport("ATSApi.dll")]
        public static extern UInt32 AlazarBoardsInSystemBySystemHandle(IntPtr systemHandle);

        [DllImport("ATSApi.dll")]
        public static extern IntPtr AlazarGetBoardBySystemID(UInt32 sid, UInt32 brdNum);

        [DllImport("ATSApi.dll")]
        public static extern IntPtr AlazarGetBoardBySystemHandle(IntPtr systemHandle, UInt32 brdNum);

        [DllImport("ATSApi.dll")]
        public static extern unsafe UInt32 AlazarQueryCapability(IntPtr h, UInt32 request, UInt32 value, UInt32* retValue);

        [DllImport("AtsApi.dll")]
        [return: MarshalAs(UnmanagedType.LPStr)]
        public static extern string AlazarErrorToText(UInt32 retCode);

        [DllImport("ATSApi.dll")]
        public static extern unsafe UInt32 AlazarGetParameter(IntPtr h, byte channel, UInt32 parameter, Int32* retValue);

        [DllImport("ATSApi.dll")]
        public static extern unsafe UInt32 AlazarGetParameterUL(IntPtr h, byte channel, UInt32 parameter, UInt32* retValue);

        #endregion

        #region - Board Configuration ---------------------------------

        [DllImport("ATSApi.dll")]
        public static extern UInt32 AlazarSetCaptureClock(IntPtr h, UInt32 Source, UInt32 Rate, UInt32 Edge, UInt32 Decimation);

        [DllImport("ATSApi.dll")]
        public static extern UInt32 AlazarSetExternalClockLevel(IntPtr h, float level_percent);

        [DllImport("ATSApi.dll")]
        public static extern UInt32 AlazarInputControl(IntPtr h, System.UInt32 Channel, UInt32 Coupling, UInt32 InputRange, UInt32 Impedance);

        [DllImport("ATSApi.dll")]
        public static extern UInt32 AlazarSetPosition(IntPtr h, UInt32 Channel, int PMPercent, UInt32 InputRange);

        [DllImport("ATSApi.dll")]
        public static extern UInt32 AlazarSetExternalTrigger(IntPtr h, UInt32 Coupling, UInt32 Range);

        [DllImport("ATSApi.dll")]
        public static extern UInt32 AlazarSetTriggerDelay(IntPtr h, UInt32 Delay);

        [DllImport("ATSApi.dll")]
        public static extern UInt32 AlazarSetTriggerTimeOut(IntPtr h, UInt32 to_ns);

        [DllImport("ATSApi.dll")]
        public static extern UInt32 AlazarTriggerTimedOut(IntPtr h);

        [DllImport("ATSApi.dll")]
        public static extern UInt32 AlazarSetTriggerOperation(IntPtr h, UInt32 TriggerOperation
                                                      , UInt32 TriggerEngine1/*j,K*/, UInt32 Source1, UInt32 Slope1, UInt32 Level1
                                                      , UInt32 TriggerEngine2/*j,K*/, UInt32 Source2, UInt32 Slope2, UInt32 Level2);

        [DllImport("ATSApi.dll")]
        public static extern UInt32 AlazarSetBWLimit(IntPtr h, UInt32 Channel, UInt32 enable);

        [DllImport("ATSApi.dll")]
        public static extern UInt32 AlazarSleepDevice(IntPtr h, UInt32 state);

        [DllImport("ATSApi.dll")]
        public static extern UInt32 AlazarSetLED(IntPtr h, UInt32 state);

        [DllImport("ATSApi.dll")]
        public static extern UInt32 AlazarConfigureAuxIO(IntPtr board, UInt32 mode, UInt32 parameter);

        [DllImport("ATSApi.dll")]
        public static extern UInt32 AlazarSetParameter(IntPtr hDevice, byte channel, UInt32 parameter, Int32 value);

        [DllImport("ATSApi.dll")]
        public static extern UInt32 AlazarSetParameterUL(IntPtr hDevice, byte channel, UInt32 parameter, UInt32 value);

        #endregion

        #region - General Acquisition ---------------------------------

        [DllImport("ATSApi.dll")]
        public static extern UInt32 AlazarStartCapture(IntPtr handle);

        [DllImport("ATSApi.dll")]
        public static extern UInt32 AlazarBusy(IntPtr h);

        [DllImport("ATSApi.dll")]
        public static extern UInt32 AlazarTriggered(IntPtr h);

        [DllImport("ATSApi.dll")]
        public static extern UInt32 AlazarSetRecordSize(IntPtr h, UInt32 PreSize, UInt32 PostSize);

        [DllImport("ATSApi.dll")]
        public static extern UInt32 AlazarForceTrigger(IntPtr handle);

        [DllImport("ATSApi.dll")]
        public static extern UInt32 AlazarForceTriggerEnable(IntPtr handle);

        [DllImport("ATSApi.dll")]
        public static extern UInt32 AlazarResetTimeStamp(IntPtr h, UInt32 resetFlag);

        #endregion

        #region - Single-Port DMA -------------------------------------

        [DllImport("ATSApi.dll")]
        public unsafe static extern UInt32 AlazarRead(IntPtr h, UInt32 Channel, void* Buf, Int32 ElementSize, Int32 Record, Int32 TransferOffset, UInt32 TransferLength);

        [DllImport("ATSApi.dll")]
        public unsafe static extern UInt32 AlazarHyperDisp(IntPtr h, void* Buffer, UInt32 BufferSize, byte* ViewBuffer, UInt32 ViewBufferSize, UInt32 NumOfPixels, UInt32 Option, UInt32 ChannelSelect, UInt32 Record, int TransferOffset, UInt32 * error);

        [DllImport("ATSApi.dll")]
        public static extern UInt32 AlazarDetectMultipleRecord(IntPtr h);

        [DllImport("ATSApi.dll")]
        public static extern UInt32 AlazarSetRecordCount(IntPtr h, UInt32 Count);

        [DllImport("ATSApi.dll")]
        public static extern UInt32 AlazarGetStatus(IntPtr h);

        [DllImport("ATSApi.dll")]
        public static extern UInt32 AlazarAbortCapture(IntPtr handle);

        [DllImport("ATSApi.dll")]
        public static extern UInt32 AlazarMaxSglTransfer(UInt32 bt);

        [DllImport("ATSApi.dll")]
        public static extern unsafe UInt32 AlazarGetMaxRecordsCapable(IntPtr h, UInt32 RecordLength, UInt32* num);

        [DllImport("ATSApi.dll")]
        public static extern UInt32 AlazarGetWhoTriggeredBySystemHandle(IntPtr systemHandle, UInt32 brdNum, UInt32 recNum);

        [DllImport("ATSApi.dll")]
        public static extern UInt32 AlazarGetWhoTriggeredBySystemID(UInt32 sid, UInt32 brdNum, UInt32 recNum);

        [DllImport("ATSApi.dll")]
        public static extern unsafe UInt32 AlazarGetTriggerAddress(IntPtr h, UInt32 Record, UInt32* TriggerAddress, UInt32* TimeStampHighPart, UInt32* TimeStampLowPart);

        #endregion

        #region - Dual-Port Asynchronous AutoDMA ----------------------

        [DllImport("ATSApi.dll")]
        public static extern UInt32 AlazarBeforeAsyncRead(IntPtr BoardHandle, UInt32 ChannelSelect, Int32 TransferOffset, UInt32 SamplesPerRecord, UInt32 RecordsPerBuffer, UInt32 RecordsPerAcquisition, UInt32 AutoDmaFlags);

        [DllImport("ATSApi.dll")]
        public static extern UInt32 AlazarAbortAsyncRead(IntPtr BoardHandle);

        [DllImport("ATSApi.dll")]
        public static extern unsafe UInt32 AlazarPostAsyncBuffer(IntPtr BoardHandle, void* Buffer, UInt32 BufferSize);

        [DllImport("ATSApi.dll")]
        public static extern unsafe UInt32 AlazarWaitAsyncBufferComplete(IntPtr BoardHandle, void* Buffer, UInt32 Timeout_ms);

        [DllImport("ATSApi.dll")]
        public static extern unsafe UInt32 AlazarWaitNextAsyncBufferComplete(IntPtr BoardHandle, void* Buffer, UInt32 BufferLength_bytes, UInt32 Timeout_ms);

        [DllImport("ATSApi.dll")]
        public static extern UInt32 AlazarCreateStreamFileA(IntPtr BoardHandle, [MarshalAs(UnmanagedType.LPStr)] string Path);

        [DllImport("ATSApi.dll")]
        public static extern UInt32 AlazarCreateStreamFileW(IntPtr BoardHandle, [MarshalAs(UnmanagedType.LPWStr)] string Path);

        #endregion

        #region - Dual-Port Synchronous AutoDMA -----------------------

        [DllImport("ATSApi.dll")]
        [ObsoleteAttribute("AlazarStartAutoDMA has been deprecated.")]
        public static extern unsafe UInt32 AlazarStartAutoDMA(IntPtr h, void* Buffer1, UInt32 UseHeader, UInt32 ChannelSelect, Int32 TransferOffset, UInt32 TransferLength, Int32 RecordsPerBuffer, Int32 RecordCount, AUTODMA_STATUS* error, UInt32 r1, UInt32 r2, UInt32* r3, UInt32* r4);

        [DllImport("ATSApi.dll")]
        [ObsoleteAttribute("AlazarGetNextAutoDMABuffer has been deprecated.")]
        public static extern unsafe UInt32 AlazarGetNextAutoDMABuffer(IntPtr h, void* Buffer1, void* Buffer2, Int32* WhichOne, Int32* RecordsTransfered, AUTODMA_STATUS* error, UInt32 r1, UInt32 r2, Int32* TriggersOccurred, UInt32* r4);

        [DllImport("ATSApi.dll")]
        [ObsoleteAttribute("AlazarGetNextBuffer has been deprecated.")]
        public static extern unsafe UInt32 AlazarGetNextBuffer(IntPtr h, void* Buffer1, void* Buffer2, Int32* WhichOne, Int32* RecordsTransfered, AUTODMA_STATUS* error, UInt32 r1, UInt32 r2, Int32* TriggersOccurred, UInt32* r4);

        [DllImport("ATSApi.dll")]
        [ObsoleteAttribute("AlazarCloseAUTODma has been deprecated.")]
        public static extern UInt32 AlazarCloseAUTODma(IntPtr h);

        [DllImport("ATSApi.dll")]
        [ObsoleteAttribute("AlazarAbortAutoDMA has been deprecated.")]
        public static extern unsafe UInt32 AlazarAbortAutoDMA(IntPtr h, void* Buffer, AUTODMA_STATUS* error, UInt32 r1, UInt32 r2, UInt32* r3, UInt32* r4);

        [DllImport("ATSApi.dll")]
        public static extern unsafe UInt32 AlazarGetAutoDMAHeaderValue(IntPtr h, UInt32 Channel, void* DataBuffer, UInt32 Record, UInt32 Parameter, AUTODMA_STATUS* error);

        [DllImport("ATSApi.dll")]
        public static extern unsafe float AlazarGetAutoDMAHeaderTimeStamp(IntPtr h, UInt32 Channel, void* DataBuffer, UInt32 Record, AUTODMA_STATUS* error);

        [DllImport("ATSApi.dll")]
        public static extern unsafe void* AlazarGetAutoDMAPtr(IntPtr h, UInt32 DataOrHeader, UInt32 Channel, void* DataBuffer, UInt32 Record, AUTODMA_STATUS* error);

        [DllImport("ATSApi.dll")]
        [ObsoleteAttribute("AlazarWaitForBufferReady has been deprecated.")]
        public static extern UInt32 AlazarWaitForBufferReady(IntPtr h, Int32 tms);

        [DllImport("ATSApi.dll")]
        [ObsoleteAttribute("AlazarEvents has been deprecated.")]
        public static extern UInt32 AlazarEvents(IntPtr h, UInt32 enable);

        #endregion

        #region - Other -----------------------------------------------

        [DllImport("ATSApi.dll")]
        public static extern unsafe UInt32 AlazarGetOEMFPGAName(int opcodeID, [MarshalAs(UnmanagedType.LPStr)] string FullPath, UInt32* error);

        [DllImport("ATSApi.dll")]
        public static extern unsafe UInt32 AlazarOEMSetWorkingDirectory([MarshalAs(UnmanagedType.LPStr)] string wDir, UInt32* error);

        [DllImport("ATSApi.dll")]
        public static extern unsafe UInt32 AlazarOEMGetWorkingDirectory([MarshalAs(UnmanagedType.LPStr)] string wDir, UInt32* error);

        [DllImport("ATSApi.dll")]
        public static extern unsafe UInt32 AlazarParseFPGAName([MarshalAs(UnmanagedType.LPStr)] string FullName, byte* Name, UInt32* Type1, UInt32* MemSize, UInt32* MajVer, UInt32* MinVer, UInt32* MajRev, UInt32* MinRev, UInt32* error);

        [DllImport("ATSApi.dll")]
        public static extern unsafe UInt32 AlazarOEMDownLoadFPGA(IntPtr h, [MarshalAs(UnmanagedType.LPStr)] string FileName, UInt32* RetValue);

        [DllImport("ATSApi.dll")]
        public static extern unsafe UInt32 AlazarDownLoadFPGA(IntPtr h, [MarshalAs(UnmanagedType.LPStr)] string FileName, UInt32* RetValue);

        [DllImport("ATSApi.dll")]
        public static extern UInt32 AlazarWriteRegister(IntPtr hDevice, UInt32 offset, UInt32 Val, UInt32 pswrd);

        [DllImport("ATSApi.dll")]
        public static extern unsafe UInt32 AlazarReadRegister(IntPtr hDevice, UInt32 offset, UInt32* retVal, UInt32 pswrd);

        #endregion

        #endregion
    }
}
