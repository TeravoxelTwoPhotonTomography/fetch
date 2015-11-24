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
#include "vibratome.pb.h"

#define CHKLBL(expr,lbl) \
  if(!(expr))                                                                                             \
  { warning("Device [Vibratome] - Expression failed."ENDL"\t%s(%d)"ENDL"\t%s"ENDL,__FILE__,__LINE__,#expr); \
    goto lbl;                                                                                         \
  }
#define CHK(expr) \
  if(!(expr))                                                                                             \
  { warning("Device [Vibratome] - Expression failed."ENDL"\t%s(%d)"ENDL"\t%s"ENDL,__FILE__,__LINE__,#expr); \
    goto Error;                                                                                         \
  }

namespace fetch  { 

  bool operator==(const cfg::device::SerialControlVibratome& a, const cfg::device::SerialControlVibratome& b)  {return equals(&a,&b);}
  bool operator==(const cfg::device::SimulatedVibratome& a, const cfg::device::SimulatedVibratome& b)          {return equals(&a,&b);}
  bool operator==(const cfg::device::Vibratome& a, const cfg::device::Vibratome& b)                            {return equals(&a,&b);}

  bool operator!=(const cfg::device::SerialControlVibratome& a, const cfg::device::SerialControlVibratome& b)  {return !(a==b);}
  bool operator!=(const cfg::device::SimulatedVibratome& a, const cfg::device::SimulatedVibratome& b)          {return !(a==b);}
  bool operator!=(const cfg::device::Vibratome& a, const cfg::device::Vibratome& b)                            {return !(a==b);}

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
      warning("%s(%d): Vibratome's COM port wasn't closed properly."ENDL,__FILE__,__LINE__); // should've been close in on_detach()
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
  { debug("Vibratome: on_attach"ENDL);
    Config c = get_config();
    unsigned int sts = 0;
    COMMTIMEOUTS timeouts={0};                      // all times are in ms
    DCB dcb={0};

    CHKLBL( INVALID_HANDLE_VALUE!=( 
        _h=CreateFile(c.port().c_str()              // file name
                     ,GENERIC_READ | GENERIC_WRITE  // desired access
                     ,0                             // share mode
                     ,0                             // security attributes
                     ,OPEN_EXISTING                 // creation disposition
                     ,FILE_ATTRIBUTE_NORMAL         // flags and attr
                     ,0)                            // template file
        ),ESERIAL);

    dcb.DCBlength=sizeof(dcb);

    Guarded_Assert_WinErr( GetCommState(_h,&dcb) );
    dcb.BaudRate = closestCBR( c.baud() );          //  baud rate               
    dcb.ByteSize = 8;                               //  data size, xmit and rcv 
    dcb.Parity   = NOPARITY;                        //  parity bit              
    dcb.StopBits = ONESTOPBIT;                      //  stop bit                
    Guarded_Assert_WinErr( SetCommState(_h,&dcb) );
    
    timeouts.ReadIntervalTimeout         =50;
    timeouts.ReadTotalTimeoutMultiplier  =10;
    timeouts.ReadTotalTimeoutConstant    =50;
    timeouts.WriteTotalTimeoutMultiplier =50;
    timeouts.WriteTotalTimeoutConstant   =10;
    Guarded_Assert_WinErr( SetCommTimeouts(_h,&timeouts) );

    CHKLBL(AMP(_config->amplitude()),EAMP);
    _lastAttachedPort = c.port();
    stop(); // In case it was turned on via another serial interface (e.g. PuTTY). Want consistent initial state.
    return 0;
EAMP:
    _close();
ESERIAL:
    _lastAttachedPort.clear();
    return 1; // failure
  }
  
  unsigned int SerialControlVibratome::_close()
  { debug("Vibratome: _close"ENDL);    
    if( _h!=INVALID_HANDLE_VALUE )
    { Guarded_Assert_WinErr( CloseHandle(_h) );
      _h = INVALID_HANDLE_VALUE;
    }
    return 0; //success
  }

  unsigned int SerialControlVibratome::on_detach()
  { stop(); 
    return _close(); 
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

  void
    SerialControlVibratome::
    onUpdate()
  { Config c = get_config();
    setAmplitude(c.amplitude());
    //ignore baud rate changes...I don't think they do anything
    if(_lastAttachedPort.compare(c.port())!=0)
    { Task *last = 0;
      if(_agent->is_armed())
      { last = _agent->_task;
        CHKLBL(_agent->disarm()==0,Error);
      }
      CHKLBL(_agent->detach()==0,Error);
      CHKLBL(_agent->attach()==0,Error);
      if(last)
        CHKLBL(_agent->arm(last,_agent->_owner)==0,Error);             
    }
  Error:
    return;    
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
  { Config c = get_config();
    return c.amplitude();
  }

  double SerialControlVibratome::getAmplitude_mm()
  { return getAmplitude_ControllerUnits()*_config->amp2mm();
  }

  // The idea here is that these class translate the serial API to a public
  // API.  That's why they're here even though they don't do anything.  It's
  // basically just in case the serial interface changes.
  int SerialControlVibratome::start()
  { debug("Vibratome: start"ENDL);
    return START();
  }

  int SerialControlVibratome::stop()
  { debug("Vibratome: stop"ENDL);
    return STOP();
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
    CHK(nn<(unsigned) *nresp);
    *nresp=nn;
    return ok;
Error:
    return 0;
  }

  int SerialControlVibratome::AMP(int val)
  { char buf[1024];
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
    _config->CopyFrom(cfg);
    _config->set_kind(kind);
    setKind(kind);
    switch(kind)
    {    
    case cfg::device::Vibratome_VibratomeType_Serial:
      //_serial->_set_config(const_cast<Config&>(cfg).mutable_serial());
      _serial->_set_config(_config->mutable_serial());
      break;
    case cfg::device::Vibratome_VibratomeType_Simulated:    
      _simulated->_set_config(_config->mutable_simulated());
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

  void Vibratome::feed_begin_pos_mm(float *x, float *y)
  { cfg::device::Point2d p = _config->geometry().cut_pos_mm();   
    *x=p.x(); *y=p.y();
  }

  int Vibratome::set_feed_begin_pos_mm(float x, float y)
  { transaction_lock();
    _config->mutable_geometry()->mutable_cut_pos_mm()->set_x(x);
    _config->mutable_geometry()->mutable_cut_pos_mm()->set_y(y);
    transaction_unlock();
    return 1;
  }

  void Vibratome::feed_end_pos_mm(float *x, float *y)
  { feed_begin_pos_mm(x,y);
    switch( _config->feed_axis() )
    { case cfg::device::Vibratome_VibratomeFeedAxis_X: (*x) += _config->feed_mm(); break;
      case cfg::device::Vibratome_VibratomeFeedAxis_Y: (*y) += _config->feed_mm(); break;
      default:
        error("%s(%d): Bad value.  Should not be here.",__FILE__,__LINE__);
    }
  }

  float Vibratome::feed_vel_mm_p_s()
  { return _config->feed_vel_mm_per_sec();
  }
  
  int Vibratome::setFeedDist_mm(double val)
  { transaction_lock();
    _config->set_feed_mm(val);    
    transaction_unlock();
    return 1;
  }
  int Vibratome::setFeedDistNoWait_mm(double val)
  { Config cfg = get_config();
    cfg.set_feed_mm(val);
    set_config_nowait(cfg);
    return 1;
  }
  double Vibratome::getFeedDist_mm()
  { return _config->feed_mm();
  }

  int Vibratome::setFeedVel_mm(double val)
  { transaction_lock();
    _config->set_feed_vel_mm_per_sec(val);    
    transaction_unlock();
    return 1;
  }
  int Vibratome::setFeedVelNoWait_mm(double val)
  { Config cfg = get_config();
    cfg.set_feed_vel_mm_per_sec(val);
    set_config_nowait(cfg);
    return 1;
  }
  double Vibratome::getFeedVel_mm()
  { return _config->feed_vel_mm_per_sec();
  }
  
  int Vibratome::setFeedAxis(Vibratome::FeedAxis val)
  { transaction_lock();
    _config->set_feed_axis(val);    
    transaction_unlock();
    return 1;
  }
  int Vibratome::setFeedAxisNoWait(Vibratome::FeedAxis val)
  { Config cfg = get_config();
    cfg.set_feed_axis(val);
    set_config_nowait(cfg);
    return 1;
  }
  Vibratome::FeedAxis Vibratome::getFeedAxis()
  { return _config->feed_axis();
  }

  int
    Vibratome::
    setVerticalOffsetNoWait(float dz_mm)
  {
    Config cfg = get_config();
    cfg.mutable_geometry()->set_dz_mm(dz_mm);
    return set_config_nowait(cfg);
  }

  int
    Vibratome::
    setVerticalOffsetNoWait(float cutting_plane_mm, float image_plane_mm)
  {
    Config cfg = get_config();
    cfg.mutable_geometry()->set_dz_mm(image_plane_mm-cutting_plane_mm);
    return set_config_nowait(cfg);
  }

  int
    Vibratome::
    setThicknessUmNoWait(float um)
  {
    Config cfg = get_config();
    cfg.set_cut_thickness_um(um);
    return set_config_nowait(cfg);
  }

  


} //end device namespace
}   //end fetch  namespace
