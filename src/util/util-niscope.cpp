#include "config.h"
#ifdef HAVE_NISCOPE

#include "common.h"
#include "niScope.h"
#include "niModInst.h"

#define DIGCHK( expr ) (niscope_chk( vi, expr, #expr, __FILE__, __LINE__, error   ))

ViStatus niscope_chk ( ViSession vi, 
                       ViStatus result, 
                       const char *expression,
                       const char *file,
                       const int line,
                       pf_reporter report)
{ ViChar   errorSource[MAX_FUNCTION_NAME_SIZE] = "\0";
  ViChar   errorMessage[MAX_ERROR_DESCRIPTION] = "\0";

  if (result != VI_SUCCESS)
  { niScope_errorHandler (vi, result, errorSource, errorMessage);
    (*report)( "(%s:%d) %s\r\n%s\r\n", file, line, (expression), errorMessage );
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

  DIGCHK( niScope_GetAttributeViReal64( vi, NULL, NISCOPE_ATTR_BACKLOG, &pts ));
  debug("Digitizer                              Backlog: %4.1f MS\r\n",pts/1024.0/1024.0);

  DIGCHK( niScope_GetAttributeViInt32( vi, NULL, NISCOPE_ATTR_ONBOARD_MEMORY_SIZE, &mem ));
  debug("Digitizer                          Buffer size: %4.1f MB\r\n",mem/1024.0/1024.0);

  DIGCHK( niScope_GetAttributeViInt32( vi, NULL, NISCOPE_ATTR_DATA_TRANSFER_BLOCK_SIZE, &mem ));
  debug("Digitizer             Data Transfer Block size: %4.1f MB\r\n",mem/1024.0/1024.0);

  DIGCHK( niScope_GetAttributeViReal64( vi, NULL, NISCOPE_ATTR_DATA_TRANSFER_MAXIMUM_BANDWIDTH, &pts ));
  debug("Digitizer Data Transfer Maximum Bandwidth size: %4.1f MB\r\n",pts/1024.0/1024.0);

  DIGCHK( niScope_GetAttributeViInt32( vi, NULL, NISCOPE_ATTR_DATA_TRANSFER_PREFERRED_PACKET_SIZE, &mem ));
  debug("Digitizer  Data Transfer Preferred Packet size: %4.1f MB\r\n",mem/1024.0/1024.0);

  DIGCHK( niScope_GetAttributeViReal64( vi, NULL, NISCOPE_ATTR_MAX_REAL_TIME_SAMPLING_RATE, &pts ));
  debug("Digitizer     Data Max real time sampling rate: %4.1f MHz\r\n",pts/1024.0/1024.0);

  DIGCHK( niScope_GetAttributeViReal64( vi, NULL, NISCOPE_ATTR_HORZ_SAMPLE_RATE, &pts ));
  debug("Digitizer                 actual sampling rate: %4.1f MHz\r\n",pts/1024.0/1024.0);

  DIGCHK( niScope_GetAttributeViReal64( vi, NULL, NISCOPE_ATTR_DEVICE_TEMPERATURE, &pts ));
  debug("Digitizer                          Temperature: %4.1f C\r\n",pts);
}


void niscope_cfg_rtsi_default( ViSession vi )
{
  DIGCHK( niScope_ExportSignal( vi, NISCOPE_VAL_START_TRIGGER           , "", NISCOPE_VAL_RTSI_0 ));     // d7
  DIGCHK( niScope_ExportSignal( vi, NISCOPE_VAL_REF_TRIGGER             , "", NISCOPE_VAL_RTSI_1 ));     // d6
  DIGCHK( niScope_ExportSignal( vi, NISCOPE_VAL_READY_FOR_START_EVENT   , "", NISCOPE_VAL_RTSI_2 ));     // d5
  DIGCHK( niScope_ExportSignal( vi, NISCOPE_VAL_READY_FOR_REF_EVENT     , "", NISCOPE_VAL_RTSI_3 ));     // d4
  DIGCHK( niScope_ExportSignal( vi, NISCOPE_VAL_READY_FOR_ADVANCE_EVENT , "", NISCOPE_VAL_RTSI_4 ));     // d3 
  DIGCHK( niScope_ExportSignal( vi, NISCOPE_VAL_END_OF_ACQUISITION_EVENT, "", NISCOPE_VAL_RTSI_5 ));     // d2 
  DIGCHK( niScope_ExportSignal( vi, NISCOPE_VAL_END_OF_RECORD_EVENT     , "", NISCOPE_VAL_RTSI_6 ));     // d1
  DIGCHK( niScope_ExportSignal( vi, NISCOPE_VAL_REF_CLOCK               , "", NISCOPE_VAL_RTSI_7 ));     // d0
}

double niscope_get_backlog( ViSession vi )
{ ViReal64 pts = 0;
  DIGCHK( niScope_GetAttributeViReal64( vi, NULL, NISCOPE_ATTR_BACKLOG, &pts ));  
  return pts;
}


template<class TPixel> 
 ViStatus Fetch (ViSession vi,
                              ViConstString channellist,
                              ViReal64 timeout,
                              ViInt32 numsamples,
                              TPixel* data,
                              struct niScope_wfmInfo *info);
// niScope fetch function alias
template<> 
ViStatus Fetch< i8> ( ViSession vi,
                      ViConstString channellist,
                      ViReal64 timeout,
                      ViInt32 numsamples,
                      i8* data,
                      struct niScope_wfmInfo *info) 
{return niScope_FetchBinary8(vi,channellist,timeout,numsamples,(ViInt8*)data,info);
}

template<> 
ViStatus Fetch<i16> ( ViSession vi,
                      ViConstString channellist,
                      ViReal64 timeout,
                      ViInt32 numsamples,
                      i16* data,
                      struct niScope_wfmInfo *info) 
{return niScope_FetchBinary16(vi,channellist,timeout, numsamples,(ViInt16*)data,info);
}  

template<> 
ViStatus Fetch<u16> ( ViSession vi,
                      ViConstString channellist,
                      ViReal64 timeout,
                      ViInt32 numsamples,
                      u16* data,
                      struct niScope_wfmInfo *info) 
{ error("%s(%d)-%s()\n\tUnsupported by NIScope.  Try signed 16 bit.\n",__FILE__,__LINE__,__FUNCTION__);
  return 0;
} 

#endif //HAVE_NISCOPE