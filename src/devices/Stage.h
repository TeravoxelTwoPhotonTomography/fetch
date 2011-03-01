#pragma once
#include "object.h"
#include "stage.pb.h" 
#include "agent.h"

namespace fetch {
namespace device {

  struct StageAxisTravel  { float min,max; };
  struct StageTravel      { StageAxisTravel x,y,z; };

  class IStage
  {
    public:      
      virtual void getTravel         ( StageTravel* out)                = 0;
      virtual void getVelocity       ( float *vx, float *vy, float *vz) = 0;
      virtual int  setVelocity       ( float vx, float vy, float vz)    = 0;
      virtual void setVelocityNoWait ( float vx, float vy, float vz)    = 0;
      virtual void getPos            ( float *x, float *y, float *z)    = 0;
      virtual int  setPos            ( float  x, float  y, float  z)    = 0;
      virtual void setPosNoWait      ( float  x, float  y, float  z)    = 0;

      //Move Relative
  };

  template<class T>
    class StageBase:public IStage, public IConfigurableDevice<T>
  {
    public:
      StageBase(Agent *agent) : IConfigurableDevice<T>(agent) {}
      StageBase(Agent *agent, Config *cfg) :IConfigurableDevice<T>(agent,cfg) {}
  };

  class C843Stage:public StageBase<cfg::device::C843StageController>
  {
    public:
      C843Stage(Agent *agent);
      C843Stage(Agent *agent, Config *cfg);

      virtual unsigned int on_attach() {/**TODO**/ return 0;}
      virtual unsigned int on_detach() {/**TODO**/ return 0;}

      virtual void getTravel         ( StageTravel* out)                   {/**TODO**/}
      virtual void getVelocity       ( float *vx, float *vy, float *vz);
      virtual int  setVelocity       ( float vx, float vy, float vz);
      virtual void setVelocityNoWait ( float vx, float vy, float vz)       {Config c = get_config(); /**TODO**/ Guarded_Assert_WinErr(set_config_nowait(c));}
      virtual void getPos            ( float *x, float *y, float *z);
      virtual int  setPos            ( float  x, float  y, float  z);
      virtual void setPosNoWait      ( float  x, float  y, float  z)       {Config c = get_config(); /**TODO**/ Guarded_Assert_WinErr(set_config_nowait(c));}
  };

  class SimulatedStage:public StageBase<cfg::device::SimulatedStage>
  {   float x_,y_,z_,vx_,vy_,vz_;
    public:
      SimulatedStage(Agent *agent);
      SimulatedStage(Agent *agent, Config *cfg);  

      virtual unsigned int on_attach() {/**TODO**/ return 0;}
      virtual unsigned int on_detach() {/**TODO**/ return 0;}

      virtual void getTravel         ( StageTravel* out);
      virtual void getVelocity       ( float *vx, float *vy, float *vz);
      virtual int  setVelocity       ( float vx, float vy, float vz);
      virtual void setVelocityNoWait ( float vx, float vy, float vz)       {setVelocity(vx,vy,vz);}
      virtual void getPos            ( float *x, float *y, float *z);
      virtual int  setPos            ( float  x, float  y, float  z);
      virtual void setPosNoWait      ( float  x, float  y, float  z)       {setPos(x,y,z);}
  };

  class Stage:public StageBase<cfg::device::Stage>
  {
    C843Stage      *_c843;
    SimulatedStage *_simulated;
    IDevice        *_idevice;
    IStage         *_istage;
    public:
      Stage(Agent *agent);
      Stage(Agent *agent, Config *cfg);

      void setKind(Config::StageType kind);

      virtual unsigned int on_attach() {return _idevice->on_attach();}
      virtual unsigned int on_detach() {return _idevice->on_detach();}
      void _set_config( Config IN *cfg );
      void _set_config( const Config &cfg );

      virtual void getTravel         ( StageTravel* out)                  {_istage->getTravel(out);}
      virtual void getVelocity       ( float *vx, float *vy, float *vz)   {_istage->getVelocity(vx,vy,vz);}
      virtual int  setVelocity       ( float vx, float vy, float vz)      {return _istage->setVelocity(vx,vy,vz);}
      virtual void setVelocityNoWait ( float vx, float vy, float vz)      {_istage->setVelocityNoWait(vx,vy,vz);}
      virtual void getPos            ( float *x, float *y, float *z)      {_istage->getPos(x,y,z);}
      virtual int  setPos            ( float  x, float  y, float  z)      {return _istage->setPos(x,y,z);}
      virtual void setPosNoWait      ( float  x, float  y, float  z)      {_istage->setPosNoWait(x,y,z);}
  };
  // end namespace fetch::Device
}}

