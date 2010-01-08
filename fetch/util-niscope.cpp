#include "stdafx.h"
#include "niScope.h"

#include "niModInst.h"

ViStatus niscope_chk ( ViSession vi, 
                       ViStatus result, 
                       const char *expression,
                       pf_reporter report)
{ ViChar   errorSource[MAX_FUNCTION_NAME_SIZE] = "\0";
  ViChar   errorMessage[MAX_ERROR_DESCRIPTION] = "\0";

  if (result != VI_SUCCESS)
  { niScope_errorHandler (vi, result, errorSource, errorMessage);
    (*report)( "%s\r\n%s\r\n", (expression), errorMessage );
  }
  return result;
}

void niscope_log_wfminfo( pf_reporter pfOutput, niScope_wfmInfo *info )
{
   (*pfOutput)
         ("--\r\n"
          "ViReal64 absoluteInitialX    %g\r\n"
          "ViReal64 relativeInitialX    %g\r\n"
          "ViReal64 xIncrement          %g\r\n"
          "ViInt32  actualSamples       %d\r\n"
          "ViReal64 offset              %g\r\n"
          "ViReal64 gain                %g\r\n"
          "ViReal64 reserved1           %g\r\n"
          "ViReal64 reserved2           %g\r\n"
          "--\r\n"
          , info->absoluteInitialX
          , info->relativeInitialX
          , info->xIncrement
          , info->actualSamples
          , info->offset
          , info->gain
          , info->reserved1
          , info->reserved2 );
}

void niscope_debug_list_devices(void)
{ ViInt32 ndevices;
  ViSession session;
  Guarded_Assert( 
    VI_SUCCESS == niModInst_OpenInstalledDevicesSession("niScope",&session, &ndevices));
  debug("niModInst: found %d devices.\r\n",ndevices);
  while(ndevices--)
  { ViInt32 slot, chassis, bus, socket;
    char name[1024], model[1024], number[1024];
    niModInst_GetInstalledDeviceAttributeViInt32 (session,  //ViSession handle,
                                                  ndevices, //ViInt32 index,
                                                  NIMODINST_ATTR_SLOT_NUMBER, //ViInt32 attributeID,
                                                  &slot );  //ViInt32* attributeValue)
    niModInst_GetInstalledDeviceAttributeViInt32 (session,  //ViSession handle,
                                                  ndevices, //ViInt32 index,
                                                  NIMODINST_ATTR_CHASSIS_NUMBER, //ViInt32 attributeID,
                                                  &chassis );  //ViInt32* attributeValue)
    niModInst_GetInstalledDeviceAttributeViInt32 (session,  //ViSession handle,
                                                  ndevices, //ViInt32 index,
                                                  NIMODINST_ATTR_BUS_NUMBER, //ViInt32 attributeID,
                                                  &bus );  //ViInt32* attributeValue);
    niModInst_GetInstalledDeviceAttributeViInt32 (session,  //ViSession handle,
                                                  ndevices, //ViInt32 index,
                                                  NIMODINST_ATTR_SOCKET_NUMBER, //ViInt32 attributeID,
                                                  &socket );  //ViInt32* attributeValue);
    niModInst_GetInstalledDeviceAttributeViString ( session,  //ViSession handle,
                                                    ndevices, //ViInt32 index,
                                                    NIMODINST_ATTR_DEVICE_NAME, //ViInt32 attributeID,
                                                    1024,     //ViInt32 attributeValueBufferSize,
                                                    name );   //ViChar attributeValue[]);
    niModInst_GetInstalledDeviceAttributeViString ( session,  //ViSession handle,
                                                    ndevices, //ViInt32 index,
                                                    NIMODINST_ATTR_DEVICE_MODEL, //ViInt32 attributeID,
                                                    1024,     //ViInt32 attributeValueBufferSize,
                                                    model );   //ViChar attributeValue[]);
    niModInst_GetInstalledDeviceAttributeViString ( session,  //ViSession handle,
                                                    ndevices, //ViInt32 index,
                                                    NIMODINST_ATTR_SERIAL_NUMBER, //ViInt32 attributeID,
                                                    1024,     //ViInt32 attributeValueBufferSize,
                                                    number ); //ViChar attributeValue[]);
   
    debug("%2d:\r\n"
          "     Name: %s\r\n"
          "    Model: %s\r\n"
          "       SN: %s\r\n"
          "     Slot: %d\r\n"
          "  Chassis: %d\r\n"
          "      Bus: %d\r\n"
          "   Socket: %d\r\n",
          ndevices, name, model, number, slot, chassis, bus, socket);
    
  }             
  niModInst_CloseInstalledDevicesSession(session);
}

void niscope_debug_print_status( ViSession vi )
{ ViReal64 pts = 0;
  ViInt32 mem = 0;

  CheckPanic( niScope_GetAttributeViReal64( vi, NULL, NISCOPE_ATTR_BACKLOG, &pts ));
  debug("Digitizer Backlog: %4.1f MS\r\n",pts/1024.0/1024.0);

  CheckPanic( niScope_GetAttributeViInt32( vi, NULL, NISCOPE_ATTR_ONBOARD_MEMORY_SIZE, &mem ));
  debug("Digitizer                          Buffer size: %4.1f MB\r\n",mem/1024.0/1024.0);

  CheckPanic( niScope_GetAttributeViInt32( vi, NULL, NISCOPE_ATTR_DATA_TRANSFER_BLOCK_SIZE, &mem ));
  debug("Digitizer             Data Transfer Block size: %4.1f MB\r\n",mem/1024.0/1024.0);

  CheckPanic( niScope_GetAttributeViReal64( vi, NULL, NISCOPE_ATTR_DATA_TRANSFER_MAXIMUM_BANDWIDTH, &mem ));
  debug("Digitizer Data Transfer Maximum Bandwidth size: %4.1f MB\r\n",mem/1024.0/1024.0);

  CheckPanic( niScope_GetAttributeViInt32( vi, NULL, NISCOPE_ATTR_DATA_TRANSFER_PREFERRED_PACKET_SIZE, &mem ));
  debug("Digitizer   Data Transfer Prefered Packet size: %4.1f MB\r\n",mem/1024.0/1024.0);

  CheckPanic( niScope_GetAttributeViReal64( vi, NULL, NISCOPE_ATTR_MAX_REAL_TIME_SAMPLING_RATE, &mem ));
  debug("Digitizer     Data Max real time sampling rate: %4.1f MHz\r\n",mem/1024.0/1024.0);

  CheckPanic( niScope_GetAttributeViReal64( vi, NULL, NISCOPE_ATTR_HORZ_SAMPLE_RATE, &mem ));
  debug("Digitizer                 actual sampling rate: %4.1f MHz\r\n",mem/1024.0/1024.0);

  CheckPanic( niScope_GetAttributeViReal64( vi, NULL, NISCOPE_ATTR_DEVICE_TEMPERATURE, &mem ));
  debug("Digitizer                          Temperature: %4.1f C\r\n",mem);
}