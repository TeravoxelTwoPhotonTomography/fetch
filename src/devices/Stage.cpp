                       /*
* Stage.cpp
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


#include <list>
#include <string>
#include <sstream>

#include "stage.h"
#include "task.h"
#include "tiling.h"

#include "C843_GCS_DLL.H"

#define DAQWRN( expr )        (Guarded_DAQmx( (expr), #expr, __FILE__, __LINE__, warning))
#define DAQERR( expr )        (Guarded_DAQmx( (expr), #expr, __FILE__, __LINE__, error  ))
#define DAQJMP( expr )        goto_if_fail( 0==DAQWRN(expr), Error)

namespace fetch  {
namespace device {

     
  //
  // C843Stage
  //
  
#define C843WRN( expr )  (c843_error_handler( handle_, expr, #expr, __FILE__, __LINE__, warning ))
#define C843ERR( expr )  (c843_error_handler( handle_, expr, #expr, __FILE__, __LINE__, error   ))
#define C843JMP( expr )  goto_if(c843_error_handler( handle_, expr, #expr, __FILE__, __LINE__, warning ), Error)

  // returns 0 if no error, otherwise 1
  bool c843_error_handler(long handle, BOOL ok, const char* expr, const char* file, const int line, pf_reporter report)
  {
    char buf[1024];
    long e;
    if(handle<0)
      report(
        "(%s:%d) C843 Error %d:"ENDL
        "\t%s"ENDL
        "\tInvalid device id (Got %d)."ENDL,
        file,line,e,expr,handle);
    if(!ok)
    { e = C843_GetError(handle);

      if(C843_TranslateError(e,buf,sizeof(buf)))
      {
        report(
          "(%s:%d) C843 Error %d:"ENDL
          "\t%s"ENDL,
          "\t%s"ENDL,
          file,line,e,expr,buf);
      } else
        error(
          "(%s:%d) Problem with C843 (error %d) but error message was too long for buffer."ENDL,
          "\t%s"ENDL
          __FILE__,__LINE__,e,expr);
      return true;
    }
    return false; //this should be something corresponding to "no error"...guessing zero
  }


  C843Stage::C843Stage( Agent *agent )
   :StageBase<cfg::device::C843StageController>(agent)
   ,handle_(-1)
  {

  }

  C843Stage::C843Stage( Agent *agent, Config *cfg )
   :StageBase<cfg::device::C843StageController>(agent,cfg)
   ,handle_(-1)
  {

  }
#if 0
  //
  // EXAMPLE FROM MANUAL
  //

  char stages[1024];
  char axes[10];
  int ID;
  // connect to the C-843
  ID = C843_Connect(1);
  if (ID<0)
    return FALSE;
  // nothing is configured
  if (!C843_qCST(ID, "1234", stages, 1023))
    return FALSE;
  // the output should be
  printf("qCST() returned \"%s\"", stages);
  if (!C843_qSAI(ID, axes, 9))
    return FALSE;
  // the output should be "" - no configured axes
  printf("qSAI() returned \"%s\"", axes);
  // we want to connect two M-111.1DG to channels 1 and 2
  // and a M-112.2DG to channel four
  sprintf(stages, "M-111.1DG\nM-111.1DG\nM-111.1DG");
  if (!C843_CST(ID, "124", stages))
    return FALSE;
  if (!C843_qSAI(ID, axes, 9))
    return FALSE;
  // the output should be "124" - the new configured axes
  printf("qSAI() returned \"%s\"", axes);
  if (!C843_qCST(ID, "1234", stages, 1023))
    return FALSE;
  // the output should be "1=M-111.1DG\n2=M-111.1DG\n3=NOSTAGE\n4=M-112.2DG\n"
  printf("qCST() returned \"%s\"", stages);
  // call INI for all axes
  // "" as axes string will address all configured axes
  if (!C843_INI(ID, ""))
    return FALSE;
#endif
  //return 0 on success
  unsigned int C843Stage::on_attach()
  {
    char buf[1024];
    C843JMP( handle_=C843_Connect(_config->id()) );
    //C843JMP( C843_qCST(handle_,"1234",buf,sizeof(buf)-1) ); // expect  "1=NOSTAGE\n2=NOSTAGE\n3=NOSTAGE\n4=NOSTAGE\n"
    //C843JMP( C843_qSAI(handle_,buf,sizeof(buf)-1) ); // expect "" - the configured axes
    
    // get the axis id's and names to load
    std::stringstream ssid;                  //expect "123"
    std::stringstream ssname;                //expect "M-511.DD\nM-511.DD\nM-511.DD"
    for(int i=0;i<_config->axis_size();++i)
    { ssid << _config->axis(i).id();
      ssid << _config->axis(i).stage()<<"\n";
    }
    C843JMP( C843_CST(handle_,ssid.str().c_str(),ssname.str().c_str()) );  //configure selected axes
    C843JMP( C843_INI(handle_,""));                                        //init all configured axes

    // TODO: should do some validation.  Make sure stage name is in database and 
    //       make sure initialization happens properly.

    // TODO: Referencing?

    return 0; // ok
Error:
    return 1; // fail
  }

  // return 0 on success
  unsigned int C843Stage::on_detach()
  {
    C843_CloseConnection(handle_); // always succeeds
    handle_=-1;
    return 0; // ok
Error:
    return 1; // fail
  }

  void C843Stage::getTravel(StageTravel* out)
  { 
    /* NOTES:
       o  qTMN and q TMX I think
       o  There's a section in the manual on travel range adjustment (section 5.4)
       o  Assume that (xyz) <==> "123"
       */
    double t[3];
    C843ERR( C843_qTMN(handle_,"123",t) );
    out->x.min = t[0];
    out->y.min = t[1];
    out->z.min = t[2];

    C843ERR( C843_qTMX(handle_,"123",t) );
    out->x.max = t[0];
    out->y.max = t[1];
    out->z.max = t[2];
  }

  void C843Stage::getVelocity( float *vx, float *vy, float *vz )
  { double t[3];
    C843ERR( C843_qVEL(handle_,"123",t) );
    *vx = t[0];
    *vy = t[1];
    *vz = t[2];
  }

  int C843Stage::setVelocity( float vx, float vy, float vz )
  { const double t[3] = {vx,vy,vz};
    C843ERR( C843_VEL(handle_,"123",t) );
    return 0; // success
  }

  void C843Stage::getPos( float *x, float *y, float *z )
  { double t[3];
    C843ERR( C843_qPOS(handle_,"123",t) );
    *x = t[0];
    *y = t[1];
    *z = t[2];
  }

  int C843Stage::setPos( float x, float y, float z )
  { double t[3] = {x,y,z};
    C843ERR( C843_MOV(handle_,"123",t) );
    // TODO
    // o  do I need to block?   - see IsMoving/IsReferencing
    // o  intelligent velocity?
    // o  better error handling in case I hit limits
    // o  what is behavior around limits?
    return 0; // success
  }

     
  //
  // Simulated
  //

  SimulatedStage::SimulatedStage( Agent *agent )
    :StageBase<cfg::device::SimulatedStage>(agent)
    ,x_(0.0f)
    ,y_(0.0f)
    ,z_(0.0f)
    ,vx_(0.0f)
    ,vy_(0.0f)
    ,vz_(0.0f)
  {}

  SimulatedStage::SimulatedStage( Agent *agent, Config *cfg )
    :StageBase<cfg::device::SimulatedStage>(agent,cfg)
    ,x_(0.0f)
    ,y_(0.0f)
    ,z_(0.0f)
    ,vx_(0.0f)
    ,vy_(0.0f)
    ,vz_(0.0f)
  {}
                              
  void SimulatedStage::getTravel( StageTravel* out )
  { 
    Config c = get_config();

    out->x.min = c.axis(0).min_mm();
    out->x.max = c.axis(0).max_mm();
    out->y.min = c.axis(1).min_mm();
    out->y.max = c.axis(1).max_mm();
    out->z.min = c.axis(2).min_mm();
    out->z.max = c.axis(2).max_mm();

  }

  void SimulatedStage::getVelocity( float *vx, float *vy, float *vz )
  { *vx=vx_;
    *vy=vy_;
    *vz=vz_;
  }

  int SimulatedStage::setVelocity( float vx, float vy, float vz )
  { vx_=vx;vy_=vy;vz_=vz;
    return 1;
  }

  void SimulatedStage::getPos( float *x, float *y, float *z )
  { *x=x_;
    *y=y_;
    *z=z_;
  }

  int SimulatedStage::setPos( float x, float y, float z )
  { x_=x;y_=y;z_=z;
    return 1;
  }

  //
  // Polymorphic Stage
  //

  Stage::Stage( Agent *agent )
    :StageBase<cfg::device::Stage>(agent)
    ,_c843(NULL)
    ,_simulated(NULL)
    ,_idevice(NULL)
    ,_istage(NULL)
    ,_tiling(NULL)
    ,_fov(NULL)
  {
      setKind(_config->kind());
  }

  Stage::Stage( Agent *agent, Config *cfg )
    :StageBase<cfg::device::Stage>(agent,cfg)
    ,_c843(NULL)
    ,_simulated(NULL)
    ,_idevice(NULL)
    ,_istage(NULL)
    ,_tiling(NULL)
    ,_fov(NULL)
  {
    setKind(_config->kind());
  }   

  void Stage::setKind( Config::StageType kind )
  {
    switch(kind)
    {    
    case cfg::device::Stage_StageType_C843:
      if(!_c843)
        _c843 = new C843Stage(_agent,_config->mutable_c843());
      _idevice  = _c843;
      _istage   = _c843;
      break;
    case cfg::device::Stage_StageType_Simulated:
      if(!_simulated)
        _simulated = new SimulatedStage(_agent,_config->mutable_simulated());
      _idevice = _simulated;
      _istage  = _simulated;
      break;
    default:
      error("Unrecognized kind() for SimulatedStage.  Got: %u\r\n",(unsigned)kind);
    }
  }

  void Stage::_set_config( Config IN *cfg )
  {
    setKind(cfg->kind());
    Guarded_Assert(_c843||_simulated); // at least one device was instanced
    if(_c843)      _c843->_set_config(cfg->mutable_c843());
    if(_simulated) _simulated->_set_config(cfg->mutable_simulated());
    _config = cfg;
  }

  void Stage::_set_config( const Config &cfg )
  {                        
      cfg::device::Stage_StageType kind = cfg.kind();
      _config->set_kind(kind);
      setKind(kind);
      switch(kind)
      {    
      case cfg::device::Stage_StageType_C843:
        _c843->_set_config(cfg.c843());
        break;
      case cfg::device::Stage_StageType_Simulated:    
        _simulated->_set_config(cfg.simulated());
        break;
      default:
        error("Unrecognized kind() for SimulatedStage.  Got: %u\r\n",(unsigned)kind);
      }
  }

  // Only use when attached but disarmed.
  void Stage::_createTiling()
  { if(_tiling) {delete _tiling; _tiling=NULL;}    
    device::StageTravel travel;
    getTravel(&travel);
    if(_fov)
    {
      FieldOfViewGeometry fov = *_fov;
      _tiling = new StageTiling(travel, fov, _config->tilemode());
      _notifyTilingChanged();
    }
  }

  void Stage::addListener( StageListener *listener )
  { _listeners.insert(listener);
    if(_tiling) _tiling->addListener(listener);
  }

  void Stage::delListener( StageListener *listener )
  { _listeners.erase(listener);
    if(_tiling) _tiling->delListener(listener);
  }

  void Stage::_notifyTilingChanged()
  { TListeners::iterator i;
    for(i=_listeners.begin();i!=_listeners.end();++i)
      (*i)->tiling_changed(_tiling);  
  }

  void Stage::_notifyMoved()
  { TListeners::iterator i;
    for(i=_listeners.begin();i!=_listeners.end();++i)
      (*i)->moved();
  }

  unsigned int Stage::on_attach()
  {
    unsigned int eflag = _idevice->on_attach();
    if(eflag==0) _createTiling();
    return eflag;
  }

  unsigned int Stage::on_detach()
  {
    unsigned int eflag = _idevice->on_detach();
    if((eflag==0)&&_tiling) 
    {
      delete _tiling; 
      _tiling=NULL;
    } 
    return eflag;
  }

}} // end anmespace fetch::device
