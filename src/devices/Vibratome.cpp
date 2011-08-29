/*
* Pockels.cpp
*
*  Created on: Apr 19, 2010
*      Author: Nathan Clack <clackn@janelia.hhmi.org>
*/
/*
* Copyright 2010 Howard Hughes Medical Institute.
* All rights reserved.
* Use is subject to Janelia Farm Research Campus Software Copyright 1.1
* license terms (http://license.janelia.org/license/jfrc_copyright_1_1.html).
*/


#include "Vibratome.h"
#include "task.h"

#define CHK(expr) \
  if(!(expr))                                                                                             \
  { warning("Device [Vibratome] - Expression failed."ENDL"\t%s(%d)"ENDL"\t%s",__FILE__,__LINE__,#expr); \
    goto Error;                                                                                         \
  }

namespace fetch  {
namespace device {

  //
  // SerialControlVibratome
  //

  SerialControlVibratome::SerialControlVibratome(Agent* agent)
    : VibratomeBase<cfg::device::SerialControlVibratome>(agent)
    , _h(INVALID_HANDLE_VALUE)
  {}


  SerialControlVibratome::SerialControlVibratome(Agent* agent,Config *cfg)
    : VibratomeBase<cfg::device::SerialControlVibratome>(agent,cfg)
    , _h(INVALID_HANDLE_VALUE)
  {}

  SerialControlVibratome::~SerialControlVibratome() 
  { if(_h!=INVALID_HANDLE_VALUE)
      warning("%s(%d): Vibratome's COM port wasn't closed properly.",__FILE__,__LINE__); // should've been close in on_detach()
  }

  struct _baudrate
  { DWORD sym;
    int   bps;
  };
  static const _baudrate _rate_table[] =
  {
    {CBR_110   , 110   },
    {CBR_300   , 300   },
    {CBR_600   , 600   },
    {CBR_1200  , 1200  },
    {CBR_2400  , 2400  },
    {CBR_4800  , 4800  },
    {CBR_9600  , 9600  },
    {CBR_14400 , 14400 },
    {CBR_19200 , 19200 },
    {CBR_38400 , 38400 },
    {CBR_57600 , 57600 },
    {CBR_115200, 115200},
    {CBR_128000, 128000},
    {CBR_256000, 256000},
  };
  DWORD closestCBR(int bps)
  { const int N = sizeof(_rate_table)/sizeof(_baudrate);
    int i;
    for(i=0;i<N;++i)
      if(_rate_table[i].bps>bps)
        break;
    if(i<0) i=1;
    return _rate_table[i-1].sym;
  }

  unsigned int SerialControlVibratome::on_attach()
  { Config c = get_config();
    unsigned int sts = 0;
    Guarded_Assert_WinErr( INVALID_HANDLE_VALUE!=( 
        _h=CreateFile(c.port().c_str()              // file name
                     ,GENERIC_READ | GENERIC_WRITE  // desired access
                     ,0                             // share mode
                     ,0                             // security attributes
                     ,OPEN_EXISTING                 // creation disposition
                     ,FILE_ATTRIBUTE_NORMAL         // flags and attr
                     ,0)                            // template file
        ));

    DCB dcb = {0};
    dcb.DCBlength=sizeof(dcb);

    Guarded_Assert_WinErr( GetCommState(_h,&dcb) );
    dcb.BaudRate = closestCBR( c.baud() );          //  baud rate               
    dcb.ByteSize = 8;                               //  data size, xmit and rcv 
    dcb.Parity   = NOPARITY;                        //  parity bit              
    dcb.StopBits = ONESTOPBIT;                      //  stop bit                
    Guarded_Assert_WinErr( SetCommState(_h,&dcb) );

    COMMTIMEOUTS timeouts={0};                      // all times are in ms
    timeouts.ReadIntervalTimeout         =50;
    timeouts.ReadTotalTimeoutMultiplier  =10;
    timeouts.ReadTotalTimeoutConstant    =50;
    timeouts.WriteTotalTimeoutMultiplier =50;
    timeouts.WriteTotalTimeoutConstant   =10;
    Guarded_Assert_WinErr( SetCommTimeouts(_h,&timeouts) );
    return 0;
Error:
    _close();
    return 1; // failure
  }
  
  unsigned int SerialControlVibratome::_close()
  {
    if( _h!=INVALID_HANDLE_VALUE )
    { Guarded_Assert_WinErr( CloseHandle(_h) );
      _h = INVALID_HANDLE_VALUE;
    }
    return 0; //success
  }

  unsigned int SerialControlVibratome::on_detach()
  { return _close(); 
  }

  int SerialControlVibratome::setAmplitude(int val)
  { int ok = 0;
    CHK( isValidAmplitude(val) );
    transaction_lock();
    _config->set_amplitude(val);
    ok = AMP(val);
    transaction_unlock();
Error:
    return ok;
  }


  int SerialControlVibratome::setAmplitudeNoWait(int val)
  { int ok = 0;
    Config cfg = get_config();
    CHK(val);    
    cfg.set_amplitude(val);
    CHK(ok=AMP(val));
    set_config_nowait(cfg);
Error:
    return ok;
  }

  int SerialControlVibratome::getAmplitude_ControllerUnits()
  { return qAMP();
  }

  double SerialControlVibratome::getAmplitude_mm()
  { return qAMP()*_config->amp2mm();
  }

  // The idea here is that these class translate the serial API to a public
  // API.  That's why they're here even though they don't do anything.  It's
  // basically just in case the serial interface changes.
  int SerialControlVibratome::start()
  { return START();
  }

  int SerialControlVibratome::stop()
  { return STOP();
  }

  ///// SERIAL COMMANDS

  int SerialControlVibratome::_write(const char *buf, int n)
  { BOOL ok;
    DWORD nn;
    Guarded_Assert_WinErr__NoPanic( ok=WriteFile(_h,buf,n,&nn,NULL) );
    CHK(n==nn);
    return ok;
Error:
    return 0;
  }

  /* <*nresp> INPUT  - should be the sizeof the buffer pointed to by <resp>
   * <*nresp> OUTOUT - on success, will be the number of read bytes.
   *
   * A warning will be issued if the <resp> buffer gets filled by the read, and
   * the function will return failure.
   *
   * <cmd> and <resp> may point to the same buffer.
   */
  int SerialControlVibratome::_query(const char *cmd, int ncmd, const char *resp, int *nresp)
  { int ok;
    DWORD nn;
    CHK(ok=_write(cmd,ncmd));
    Guarded_Assert_WinErr__NoPanic( ok&= ReadFile(_h,(void*)resp,*nresp,&nn,NULL) );
    CHK(nn<*nresp);
    *nresp=nn;
    return ok;
Error:
    return 0;
  }

  int SerialControlVibratome::AMP(int val)
  { char buf[1024];
    BOOL ok;
    int n;
    memset(buf,0,sizeof(buf));
    n=snprintf(buf,sizeof(buf),"amp %d\n",val);
    return _write(buf,n);
  }

  int SerialControlVibratome::qAMP()
  { char cmd[] = "amp\n",
         buf[1024];
    int n=sizeof(buf);
    memset(buf,0,sizeof(buf));
    CHK(_query(cmd,sizeof(cmd),buf,&n));
    // Parse: should be something like "Amplitude = 250"
    int i=n;
    while(--i)
      if(buf[i]==' ')
        break;
    char *e=0;
    long v = strtol(buf+i,&e,10);
    CHK(e!=(buf+i));
    return v;
Error:
    return 0;
  }

  int SerialControlVibratome::START()
  { char buf[] = "start\n";    
    return _write(buf,sizeof(buf));
  }

  int SerialControlVibratome::STOP()
  { char buf[] = "stop\n";    
    return _write(buf,sizeof(buf));
  }

  //
  // VIBRATOME
  //
  Vibratome::Vibratome( Agent *agent )
    :VibratomeBase<cfg::device::Vibratome>(agent)
    ,_serial(NULL)
    ,_simulated(NULL)
    ,_idevice(NULL)
    ,_ivibratome(NULL)
  {
    setKind(_config->kind());
  }

  Vibratome::Vibratome( Agent *agent, Config *cfg )
    :VibratomeBase<cfg::device::Vibratome>(agent,cfg)
    ,_serial(NULL)
    ,_simulated(NULL)
    ,_idevice(NULL)
    ,_ivibratome(NULL)
  {
    setKind(cfg->kind());
  }

  Vibratome::~Vibratome()
  {
    if(_serial)    { delete _serial;    _serial=NULL; }
    if(_simulated) { delete _simulated; _simulated=NULL; }
  }

  void Vibratome::setKind( Config::VibratomeType kind )
  {
    switch(kind)
    {    
    case cfg::device::Vibratome_VibratomeType_Serial:
      if(!_serial)
        _serial = new SerialControlVibratome(_agent,_config->mutable_serial());
      _idevice    = _serial;
      _ivibratome = _serial;
      break;
    case cfg::device::Vibratome_VibratomeType_Simulated:    
      if(!_simulated)
        _simulated = new SimulatedVibratome(_agent);
      _idevice  = _simulated;
      _ivibratome = _simulated;
      break;
    default:
      error("Unrecognized kind() for Vibratome.  Got: %u\r\n",(unsigned)kind);
    }
  }

  void Vibratome::_set_config( Config IN *cfg )
  {
    setKind(cfg->kind());
    Guarded_Assert(_serial||_simulated); // at least one device was instanced
    if(_serial)    _serial->_set_config(cfg->mutable_serial());
    if(_simulated) _simulated->_set_config(cfg->mutable_simulated());
    _config = cfg;
  }

  void Vibratome::_set_config( const Config &cfg )
  {
    cfg::device::Vibratome_VibratomeType kind = cfg.kind();
    _config->set_kind(kind);
    setKind(kind);
    switch(kind)
    {    
    case cfg::device::Vibratome_VibratomeType_Serial:
      _serial->_set_config(const_cast<Config&>(cfg).mutable_serial());
      break;
    case cfg::device::Vibratome_VibratomeType_Simulated:    
      _simulated->_set_config(cfg.simulated());
      break;
    default:
      error("Unrecognized kind() for Vibratome.  Got: %u\r\n",(unsigned)kind);
    }
  }

  unsigned int Vibratome::on_attach()
  {
    Guarded_Assert(_idevice);
    return _idevice->on_attach();
  }

  unsigned int Vibratome::on_detach()
  {
    Guarded_Assert(_idevice);
    return _idevice->on_detach();
  }

} //end device namespace
}   //end fetch  namespace