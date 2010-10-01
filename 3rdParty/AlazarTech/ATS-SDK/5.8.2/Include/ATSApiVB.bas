Attribute VB_Name = "Module1"
'=============================================================================
'
' AlazarTech Inc
'
' File Name:
'
'      ATSApiVB.bas  - Version 5.5.0
'
' Copyright (c) 2006 AlazarTech Inc. All Rights Reserved.
' Unpublished - rights reserved under the Copyright laws of the
' United States And Canada.
'
' This product contains confidential information and trade secrets
' of AlazarTech Inc. Use, disclosure, or reproduction is
' prohibited without the prior express written permission of AlazarTech
' Inc
'
' Description:
'
'      Public Declarations
'
'=============================================================================
'
'********************************************************************
'Function And Subroutine Declarations
'********************************************************************
Public Declare Function VarPtr Lib "vbrun.Dll" (Var As Any) As Long
Public Declare Function AlazarOpen Lib "ATSApiVB.dll" (ByVal Name As String) As Integer
Public Declare Sub AlazarClose Lib "ATSApiVB.dll" (ByVal h As Integer)
Public Declare Function AlazarBoardsFound Lib "ATSApiVB.dll" () As Long
Public Declare Function AlazarGetCPLDVersion Lib "ATSApiVB.dll" (ByVal h As Integer, ByRef Major As Byte, ByRef Minor As Byte) As Long
Public Declare Function AlazarGetSDKVersion Lib "ATSApiVB.dll" (ByRef Major As Byte, ByRef Minor As Byte, ByRef Revision As Byte) As Long
Public Declare Function AlazarGetDriverVersion Lib "ATSApiVB.dll" (ByRef Major As Byte, ByRef Minor As Byte, ByRef Revision As Byte) As Long
Public Declare Function AlazarAbortCapture Lib "ATSApiVB.dll" (ByVal h As Integer) As Long
Public Declare Function AlazarForceTrigger Lib "ATSApiVB.dll" (ByVal h As Integer) As Long
Public Declare Function AlazarStartCapture Lib "ATSApiVB.dll" (ByVal h As Integer) As Long
Public Declare Function AlazarBusy Lib "ATSApiVB.dll" (ByVal h As Integer) As Long
Public Declare Function AlazarTriggered Lib "ATSApiVB.dll" (ByVal h As Integer) As Long
Public Declare Function AlazarGetStatus Lib "ATSApiVB.dll" (ByVal h As Integer) As Long
Public Declare Function AlazarGetChannelInfo Lib "ATSApiVB.dll" (ByVal h As Integer, ByRef MemSize As Long, ByRef SampleSize As Byte) As Long
Public Declare Function AlazarMemoryTest Lib "ATSApiVB.dll" (ByVal h As Integer, ByRef errors As Long) As Long
Public Declare Function AlazarAutoCalibrate Lib "ATSApiVB.dll" (ByVal h As Integer) As Long
Public Declare Function AlazarInputControl Lib "ATSApiVB.dll" (ByVal h As Integer, ByVal Channel As Byte, ByVal Coupling As Long, ByVal InputRange As Long, ByVal Impedance As Long) As Long
Public Declare Function AlazarSetExternalTrigger Lib "ATSApiVB.dll" (ByVal h As Integer, ByVal Coupling As Long, ByVal Range As Long) As Long
Public Declare Function AlazarSetTriggerDelay Lib "ATSApiVB.dll" (ByVal h As Integer, ByVal Delay As Long) As Long
Public Declare Function AlazarSetTriggerTimeOut Lib "ATSApiVB.dll" (ByVal h As Integer, ByVal to_ns As Long) As Long
Public Declare Function AlazarTriggerTimedOut Lib "ATSApiVB.dll" (ByVal h As Integer) As Long
Public Declare Function AlazarDetectMultipleRecord Lib "ATSApiVB.dll" (ByVal h As Integer) As Long
Public Declare Function AlazarSetTriggerOperation Lib "ATSApiVB.dll" (ByVal h As Integer, ByVal TriggerOperation As Long, ByVal TriggerEngine1 As Long, ByVal Source1 As Long, ByVal Slope1 As Long, ByVal Level1 As Long, ByVal TriggerEngine2 As Long, ByVal Source2 As Long, ByVal Slope2 As Long, ByVal Level2 As Long) As Long
Public Declare Function AlazarGetTriggerAddress Lib "ATSApiVB.dll" (ByVal h As Integer, ByVal Record As Long, ByRef TriggerAddress As Long, ByRef TimeStampHighPart As Long, ByRef TimeStampLowPart As Long) As Long
Public Declare Function AlazarSetRecordCount Lib "ATSApiVB.dll" (ByVal h As Integer, ByVal Count As Long) As Long
Public Declare Function AlazarSetRecordSize Lib "ATSApiVB.dll" (ByVal h As Integer, ByVal PreSize As Long, ByVal PostSize As Long) As Long
Public Declare Function AlazarSetCaptureClock Lib "ATSApiVB.dll" (ByVal h As Integer, ByVal Source As Long, ByVal Rate As Long, ByVal Edge As Long, ByVal Decimation As Long) As Long
Public Declare Function AlazarRead Lib "ATSApiVB.dll" (ByVal h As Integer, ByVal Channel As Long, ByRef Buffer As Any, ByVal ElementSize As Integer, ByVal Record As Long, ByVal TransferOffset As Long, ByVal TransferLength As Long) As Long
'********************************************************************
' System API
'********************************************************************
Public Declare Function AlazarGetSystemHandle Lib "ATSApiVB.dll" (ByVal sid As Integer) As Integer
Public Declare Function AlazarNumOfSystems Lib "ATSApiVB.dll" () As Long
Public Declare Function AlazarBoardsInSystemBySystemID Lib "ATSApiVB.dll" (ByVal sid As Integer) As Long
Public Declare Function AlazarBoardsInSystemByHandle Lib "ATSApiVB.dll" (ByVal systemHandle As Integer) As Long
Public Declare Function AlazarGetBoardBySystemID Lib "ATSApiVB.dll" (ByVal sid As Integer, ByVal brdNum As Long) As Integer
Public Declare Function AlazarGetBoardBySystemHandle Lib "ATSApiVB.dll" (ByVal systemHandle As Integer, ByVal brdNum As Long) As Integer
Public Declare Function AlazarSetLED Lib "ATSApiVB.dll" (ByVal h As Integer, ByVal state As Integer) As Long
Public Declare Function AlazarMaxSglTransfer Lib "ATSApiVB.dll" (ByVal bt As Integer) As Long
Public Declare Function AlazarStreamCapture Lib "ATSApiVB.dll" (ByVal h As Integer, ByRef Buffer As Any, ByVal BufferSize As Long, ByVal DeviceOption As Long, ByVal ChannelSelect As Long, ByRef error As Long) As Integer
Public Declare Function AlazarGetWhoTriggeredBySystemHandle Lib "ATSApiVB.dll" (ByVal systemHandle As Integer, ByVal brdNum As Long, ByVal recNum As Long) As Long
Public Declare Function AlazarGetWhoTriggeredBySystemID Lib "ATSApiVB.dll" (ByVal sid As Integer, ByVal brdNum As Long, ByVal recNum As Long) As Long
'********************************************************************
'AUTO DMA Related Constants, and Functions
'Available on ATS460, ATS660, And ATS860  devices
'********************************************************************
Public Const ADMA_Completed                 As Long = 0
Public Const ADMA_Buffer1Invalid            As Long = 1
Public Const ADMA_Buffer2Invalid            As Long = 2
Public Const ADMA_BoardHandleInvalid        As Long = 3
Public Const ADMA_InternalBuffer1Invalid    As Long = 4
Public Const ADMA_InternalBuffer2Invalid    As Long = 5
Public Const ADMA_OverFlow                  As Long = 6
Public Const ADMA_InvalidChannel            As Long = 7
Public Const ADMA_DMAInProgress             As Long = 8
Public Const ADMA_UseHeaderNotSet           As Long = 9
Public Const ADMA_HeaderNotValid            As Long = 10
Public Const ADMA_InvalidRecsPerBuffer      As Long = 11
'********************************************************************
' Control Flags for AutoDMA used in AlazarStartAutoDMA
Public Const ADMA_EXTERNAL_STARTCAPTURE  As Long = &H1
Public Const ADMA_TRADITIONAL_MODE       As Long = &H0
Public Const ADMA_ENABLE_RECORD_HEADERS  As Long = &H8
Public Const ADMA_SINGLE_DMA_CHANNEL     As Long = &H10
Public Const ADMA_ALLOC_BUFFERS          As Long = &H20
Public Const ADMA_CONTINUOUS_MODE        As Long = &H100
Public Const ADMA_NPT                    As Long = &H200
Public Const ADMA_TRIGGERED_STREAMING    As Long = &H400
Public Const ADMA_FIFO_ONLY_STREAMING    As Long = &H800 'ATS9462 mode
Public Declare Function AlazarStartAutoDMA Lib "ATSApiVB.dll" (ByVal h As Integer, ByRef Buffer1 As Any, ByVal UseHeader As Long, ByVal ChannelSelect As Long, ByVal TransferOffset As Integer, ByVal TransferLength As Long, ByVal RecordsPerBuffer As Long, ByVal RecordCount As Long, ByRef error As Long, ByVal r1 As Long, ByVal r2 As Long, ByRef r3 As Long, ByRef r4 As Long) As Long
'********************************************************************
Public Declare Function AlazarGetNextAutoDMABuffer Lib "ATSApiVB.dll" (ByVal h As Integer, ByRef Buffer1 As Any, ByRef Buffer2 As Any, ByRef WhichOne As Long, ByRef RecordsTransfered As Long, ByRef error As Long, ByVal r1 As Long, ByVal r2 As Long, ByRef TriggersOccurred As Long, ByRef r4 As Long) As Long
Public Declare Function AlazarGetNextBuffer Lib "ATSApiVB.dll" (ByVal h As Integer, ByRef Buffer1 As Any, ByRef Buffer2 As Any, ByRef WhichOne As Long, ByRef RecordsTransfered As Long, ByRef error As Long, ByVal r1 As Long, ByVal r2 As Long, ByRef TriggersOccurred As Long, ByRef r4 As Long) As Long
Public Declare Function AlazarCloseAUTODma Lib "ATSApiVB.dll" (ByVal h As Integer) As Long
Public Declare Function AlazarAbortAutoDMA Lib "ATSApiVB.dll" (ByVal h As Integer, ByRef Buffer1 As Any, ByRef error As Long, ByVal r1 As Long, ByVal r2 As Long, ByRef r3 As Long, ByRef r4 As Long) As Long
Public Declare Function AlazarBeforeAsyncRead Lib "ATSApiVB.dll" (ByVal h As Integer, ByVal uChannelSelect As Long, ByVal lTransferOffset As Long, ByVal uSamplesPerRecord As Long, ByVal uRecordsPerBuffer As Long, ByVal uRecordsPerAcquisition As Long, ByVal uFlags As Long) As Long
Public Declare Function AlazarWaitAsyncBufferComplete Lib "ATSApiVB.dll" (ByVal h As Integer, ByRef Buffer As Any, ByVal uTimeout_ms As Long) As Long
Public Declare Function AlazarWaitNextAsyncBufferComplete Lib "ATSApiVB.dll" (ByVal h As Integer, ByRef Buffer As Any, ByVal uBufferLength_bytes As Long, ByVal uTimeout_ms As Long) As Long
Public Declare Function AlazarAbortAsyncRead Lib "ATSApiVB.dll" (ByVal h As Integer) As Long
Public Declare Function AlazarPostAsyncBuffer Lib "ATSApiVB.dll" (ByVal h As Integer, ByRef Buffer As Any, ByVal BytesPerBuffer As Long) As Long

'********************************************************************
' Header Constants and Structures that need to be used
' when dealing with AutoDMA captures that use the header.
'********************************************************************
Public Const ADMA_CLOCKSOURCE               As Long = &H1
Public Const ADMA_CLOCKEDGE                 As Long = &H2
Public Const ADMA_SAMPLERATE                As Long = &H3
Public Const ADMA_INPUTRANGE                As Long = &H4
Public Const ADMA_INPUTCOUPLING             As Long = &H5
Public Const ADMA_IMPUTIMPEDENCE            As Long = &H6
Public Const ADMA_EXTTRIGGERED              As Long = &H7
Public Const ADMA_CHA_TRIGGERED             As Long = &H8
Public Const ADMA_CHB_TRIGGERED             As Long = &H9
Public Const ADMA_TIMEOUT                   As Long = &HA
Public Const ADMA_THISCHANTRIGGERED         As Long = &HB
Public Const ADMA_SERIALNUMBER              As Long = &HC
Public Const ADMA_SYSTEMNUMBER              As Long = &HD
Public Const ADMA_BOARDNUMBER               As Long = &HE
Public Const ADMA_WHICHCHANNEL              As Long = &HF
Public Const ADMA_SAMPLERESOLUTION          As Long = &H10
Public Const ADMA_DATAFORMAT                As Long = &H11
Public Declare Function AlazarGetAutoDMAHeaderValue Lib "ATSApiVB.dll" (ByVal h As Integer, ByVal Channel As Long, ByRef DataBuffer As Any, ByVal Record As Long, ByVal Parameter As Long, ByRef error As Long) As Long
Public Declare Function AlazarGetAutoDMAHeaderTimeStamp Lib "ATSApiVB.dll" (ByVal h As Integer, ByVal Channel As Long, ByRef DataBuffer As Any, ByVal Record As Long, ByRef error As Long) As Double
Public Declare Function AlazarWaitForBufferReady Lib "ATSApiVB.dll" (ByVal h As Integer, ByVal tms As Long) As Long
Public Declare Function AlazarEvents Lib "ATSApiVB.dll" (ByVal h As Integer, ByVal enable As Long) As Long
'********************************************************************
' Special Device operations for the ATS460, ATS660 and ATS860
'********************************************************************
Public Declare Function AlazarSetBWLimit Lib "ATSApiVB.dll" (ByVal h As Integer, ByVal Channel As Long, ByVal enable As Long) As Long
Public Declare Function AlazarSleepDevice Lib "ATSApiVB.dll" (ByVal h As Integer, ByVal state As Long) As Long
Public Declare Function AlazarReadRegister Lib "ATSApiVB.dll" (ByVal h As Integer, ByVal a As Long, ByRef b As Long, ByVal pswrd As Long) As Long
Public Declare Function AlazarWriteRegister Lib "ATSApiVB.dll" (ByVal h As Integer, ByVal a As Long, ByVal b As Long, ByVal pswrd As Long) As Long
' TimeStamp Control Api
Public Declare Function AlazarResetTimeStamp Lib "ATSApiVB.dll" (ByVal h As Integer, ByVal resetFlag As Long) As Long
'********************************************************************
' Custom/OEM Specific FPGA Image handling routines
'********************************************************************
Public Const FPGA_GETFIRST          As Long = &HFFFFFFFF
Public Const FPGA_GETNEXT           As Long = &HFFFFFFFE
Public Const FPGA_GETLAST           As Long = &HFFFFFFFC
Public Declare Function AlazarOEMDownLoadFPGA Lib "ATSApiVB.dll" (ByVal h As Integer, ByRef FileName As Any, ByRef error As Long) As Integer
Public Declare Function AlazarParseFPGAName Lib "ATSApiVB.dll" (ByRef FullPath As Any, ByRef Name As Any, ByRef bType As Long, ByRef MemSize As Long, ByRef MajVer As Long, ByRef MinVer As Long, ByRef MajRev As Long, ByRef MinRev As Long, ByRef error As Long) As Long
Public Declare Function AlazarGetOEMFPGAName Lib "ATSApiVB.dll" (ByVal opcodeID As Long, ByRef FullPath As Any, ByRef error As Long) As Long
Public Declare Function AlazarOEMGetWorkingDirectory Lib "ATSApiVB.dll" (ByRef wDir As Any, ByRef error As Long) As Long
Public Declare Function AlazarOEMSetWorkingDirectory Lib "ATSApiVB.dll" (ByRef wDir As Any, ByRef error As Long) As Long

'
' Board Definition structure
'
Type BoardDef
    RecordCount As Long
    RecLength As Long
    PreDepth As Long
    ClockSource As Long
    ClockEdge As Long
    SampleRate As Long
    CouplingChanA As Long
    InputRangeChanA As Long
    InputImpedChanA As Long
    CouplingChanB As Long
    InputRangeChanB As Long
    InputImpedChanB As Long
    TriEngOperation As Long
    TriggerEngine1 As Long
    TrigEngSource1 As Long
    TrigEngSlope1 As Long
    TrigEngLevel1 As Long
    TriggerEngine2 As Long
    TrigEngSource2 As Long
    TrigEngSlope2 As Long
    TrigEngLevel2 As Long
End Type

Public Const ATS_NONE   As Integer = 0
Public Const ATS850     As Integer = 1
Public Const ATS310     As Integer = 2
Public Const ATS330     As Integer = 3
Public Const ATS855     As Integer = 4
Public Const ATS315     As Integer = 5
Public Const ATS335     As Integer = 6
Public Const ATS460     As Integer = 7
Public Const ATS860     As Integer = 8
Public Const ATS660     As Integer = 9
Public Const ATS665     As Integer = 10
Public Const ATS9462    As Integer = 11
Public Const ASTHOLDER5 As Integer = 12
Public Const ASTHOLDER6 As Integer = 13
Public Const ASTHOLDER7 As Integer = 14
Public Const ASTHOLDER8 As Integer = 15
Public Const ATS_LAST   As Integer = 16

'
' Sample Rate values
'
Public Const SAMPLE_RATE_1KSPS        As Long = &H1
Public Const SAMPLE_RATE_2KSPS        As Long = &H2
Public Const SAMPLE_RATE_5KSPS        As Long = &H4
Public Const SAMPLE_RATE_10KSPS       As Long = &H8
Public Const SAMPLE_RATE_20KSPS       As Long = &HA
Public Const SAMPLE_RATE_50KSPS       As Long = &HC
Public Const SAMPLE_RATE_100KSPS      As Long = &HE
Public Const SAMPLE_RATE_200KSPS      As Long = &H10
Public Const SAMPLE_RATE_500KSPS      As Long = &H12
Public Const SAMPLE_RATE_1MSPS        As Long = &H14
Public Const SAMPLE_RATE_2MSPS        As Long = &H18
Public Const SAMPLE_RATE_5MSPS        As Long = &H1A
Public Const SAMPLE_RATE_10MSPS       As Long = &H1C
Public Const SAMPLE_RATE_20MSPS       As Long = &H1E
Public Const SAMPLE_RATE_25MSPS       As Long = &H21
Public Const SAMPLE_RATE_50MSPS       As Long = &H22
Public Const SAMPLE_RATE_100MSPS      As Long = &H24
Public Const SAMPLE_RATE_125MSPS      As Long = &H25
Public Const SAMPLE_RATE_160MSPS      As Long = &H26
Public Const SAMPLE_RATE_180MSPS      As Long = &H27
Public Const SAMPLE_RATE_200MSPS      As Long = &H28
Public Const SAMPLE_RATE_250MSPS      As Long = &H2B
Public Const SAMPLE_RATE_500MSPS      As Long = &H30
Public Const SAMPLE_RATE_1GSPS        As Long = &H35
' user define sample rate - used with External Clock
Public Const SAMPLE_RATE_USER_DEF     As Long = &H40
' ATS660 Specific Setting for using the PLL
'
' The base value can be used to create a PLL frequency
' in a simple manner.
'
' Ex.
'        115 MHz = PLL_10MHZ_REF_100MSPS_BASE + 15000000
'        120 MHz = PLL_10MHZ_REF_100MSPS_BASE + 20000000
Public Const PLL_10MHZ_REF_100MSPS_BASE  As Long = &H5F5E100

' ATS660 Specific Decimation constants
Public Const DECIMATE_BY_8           As Integer = &H8
Public Const DECIMATE_BY_64          As Integer = &H40

'
' Impedance Values
'
Public Const IMPEDANCE_1M_OHM         As Integer = &H1
Public Const IMPEDANCE_50_OHM         As Integer = &H2
Public Const IMPEDANCE_75_OHM         As Integer = &H4
Public Const IMPEDANCE_300_OHM        As Integer = &H8

'
' Clock Source
'
Public Const INTERNAL_CLOCK           As Integer = &H1
Public Const EXTERNAL_CLOCK           As Integer = &H2
Public Const FAST_EXTERNAL_CLOCK      As Integer = &H2
Public Const MEDIUM_EXTERNAL_CLOCK    As Integer = &H3
Public Const SLOW_EXTERNAL_CLOCK      As Integer = &H4
Public Const EXTERNAL_CLOCK_AC        As Integer = &H5 'ATS660
Public Const EXTERNAL_CLOCK_DC        As Integer = &H6 'ATS660
Public Const EXTERNAL_CLOCK_10MHz_REF As Integer = &H7 'ATS660

'
' Clock Edge
'
Public Const CLOCK_EDGE_RISING        As Integer = &H0
Public Const CLOCK_EDGE_FALLING       As Integer = &H1

'
' Input Ranges
'
Public Const INPUT_RANGE_PM_20_MV     As Integer = &H1
Public Const INPUT_RANGE_PM_40_MV     As Integer = &H2
Public Const INPUT_RANGE_PM_50_MV     As Integer = &H3
Public Const INPUT_RANGE_PM_80_MV     As Integer = &H4
Public Const INPUT_RANGE_PM_100_MV    As Integer = &H5
Public Const INPUT_RANGE_PM_200_MV    As Integer = &H6
Public Const INPUT_RANGE_PM_400_MV    As Integer = &H7
Public Const INPUT_RANGE_PM_500_MV    As Integer = &H8
Public Const INPUT_RANGE_PM_800_MV    As Integer = &H9
Public Const INPUT_RANGE_PM_1_V       As Integer = &HA
Public Const INPUT_RANGE_PM_2_V       As Integer = &HB
Public Const INPUT_RANGE_PM_4_V       As Integer = &HC
Public Const INPUT_RANGE_PM_5_V       As Integer = &HD
Public Const INPUT_RANGE_PM_8_V       As Integer = &HE
Public Const INPUT_RANGE_PM_10_V      As Integer = &HF
Public Const INPUT_RANGE_PM_20_V      As Integer = &H10
Public Const INPUT_RANGE_PM_40_V      As Integer = &H11
Public Const INPUT_RANGE_PM_16_V      As Integer = &H12
Public Const INPUT_RANGE_HIFI         As Integer = &H20

'
' Coupling Values
'
Public Const AC_COUPLING              As Integer = &H1
Public Const DC_COUPLING              As Integer = &H2

'
' Trigger Engines
'
Public Const TRIG_ENGINE_J            As Integer = &H0
Public Const TRIG_ENGINE_K            As Integer = &H1

'
' Trigger Engine Operation
'
Public Const TRIG_ENGINE_OP_J             As Integer = &H0
Public Const TRIG_ENGINE_OP_K             As Integer = &H1
Public Const TRIG_ENGINE_OP_J_OR_K        As Integer = &H2
Public Const TRIG_ENGINE_OP_J_AND_K       As Integer = &H3
Public Const TRIG_ENGINE_OP_J_XOR_K       As Integer = &H4
Public Const TRIG_ENGINE_OP_J_AND_NOT_K   As Integer = &H5
Public Const TRIG_ENGINE_OP_NOT_J_AND_K   As Integer = &H6

'
' Trigger Engine Sources
'
Public Const TRIG_CHAN_A              As Integer = &H0
Public Const TRIG_CHAN_B              As Integer = &H1
Public Const TRIG_EXTERNAL            As Integer = &H2
Public Const TRIG_DISABLE             As Integer = &H3

'
' Trigger Slope
'
Public Const TRIGGER_SLOPE_POSITIVE   As Integer = &H1
Public Const TRIGGER_SLOPE_NEGATIVE   As Integer = &H2

'
' Channel Selection
'
Public Const CHANNEL_ALL              As Integer = &H0
Public Const CHANNEL_A                As Integer = &H1
Public Const CHANNEL_B                As Integer = &H2
Public Const CHANNEL_C                As Integer = &H3
Public Const CHANNEL_D                As Integer = &H4
Public Const CHANNEL_E                As Integer = &H5
Public Const CHANNEL_F                As Integer = &H6
Public Const CHANNEL_G                As Integer = &H7
Public Const CHANNEL_H                As Integer = &H8


'
' Master/Slave Configuration
'
Public Const BOARD_IS_INDEPENDENT     As Integer = &H0
Public Const BOARD_IS_MASTER          As Integer = &H1
Public Const BOARD_IS_SLAVE           As Integer = &H2
Public Const BOARD_IS_LAST_SLAVE      As Integer = &H3

'
' LED Control
'
Public Const LED_OFF                  As Integer = &H0
Public Const LED_ON                   As Integer = &H1

'
' Attenuator Relay
'
Public Const AR_X1                    As Integer = &H0
Public Const AR_DIV40                 As Integer = &H1

'
' External Trigger Attenuator Relay
'
Public Const ETR_DIV5                 As Integer = &H0
Public Const ETR_X1                   As Integer = &H1
Public Const ETR_5V                   As Integer = &H0
Public Const ETR_1V                   As Integer = &H1

'
' Device Sleep state
'
Public Const POWER_OFF               As Integer = &H0
Public Const POWER_ON                As Integer = &H1

'
' Software Events control
'
Public Const SW_EVENTS_OFF           As Integer = &H0
Public Const SW_EVENTS_ON            As Integer = &H1

'
' TimeStamp Value Reset Control
'
Public Const TIMESTAMP_RESET_FIRSTTIME_ONLY  As Integer = &H0
Public Const TIMESTAMP_RESET_ALWAYS          As Integer = &H1

