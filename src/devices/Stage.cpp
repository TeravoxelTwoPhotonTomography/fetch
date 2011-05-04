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
#include "stage.h"
#include "task.h"

#define DAQWRN( expr )        (Guarded_DAQmx( (expr), #expr, warning))
#define DAQERR( expr )        (Guarded_DAQmx( (expr), #expr, error  ))
#define DAQJMP( expr )        goto_if_fail( 0==DAQWRN(expr), Error)

namespace fetch  {
namespace device {

     
  //
  // C843Stage
  //


  C843Stage::C843Stage( Agent *agent )
   :StageBase<cfg::device::C843StageController>(agent)   
  {

  }

  C843Stage::C843Stage( Agent *agent, Config *cfg )
   :StageBase<cfg::device::C843StageController>(agent,cfg)
  {

  }

  void C843Stage::getVelocity( float *vx, float *vy, float *vz )
  {

  }

  int C843Stage::setVelocity( float vx, float vy, float vz )
  { return 0;
  }

  void C843Stage::getPos( float *vx, float *vy, float *vz )
  {
  }

  int C843Stage::setPos( float vx, float vy, float vz )
  { return 0;
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
  {
      setKind(_config->kind());
  }

  Stage::Stage( Agent *agent, Config *cfg )
    :StageBase<cfg::device::Stage>(agent,cfg)
    ,_c843(NULL)
    ,_simulated(NULL)
    ,_idevice(NULL)
    ,_istage(NULL)
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
    if(_c843)     _c843->_set_config(cfg->mutable_c843());
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

}} // end anmespace fetch::device
