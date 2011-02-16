#include "object.h"

namespace fetch {
namespace device {

  struct StageAxisTravel  { float min,max; };
  struct StageTravel      { StageAxisTravel x,y,z; };

  class IStage
  {
    public:      
      virtual void getTravel   (StageTravel* out)=0;
      virtual void getVelocity       ( float *vx, float *vy, float *vz) = 0;
      virtual int  setVelocity       ( float vx, float vy, float vz)    = 0;
      virtual int  setVelocityNoWait ( float vx, float vy, float vz);
      virtual void getPos            ( float *vx, float *vy, float *vz) = 0;
      virtual int  setPos            ( float vx, float vy, float vz)    = 0;
      virtual int  setPosNoWait      ( float vx, float vy, float vz);
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

      virtual void getVelocity       ( float *vx, float *vy, float *vz);
      virtual int  setVelocity       ( float vx, float vy, float vz);
      virtual void getPos            ( float *vx, float *vy, float *vz);
      virtual int  setPos            ( float vx, float vy, float vz);
  };

  class SimulatedStage:public StageBase<cfg::device::SimulatedStage>
  {
    public:
      SimulatedStage(Agent *agent);
      SimulatedStage(Agent *agent, Config *cfg);

      virtual void getVelocity       ( float *vx, float *vy, float *vz);
      virtual int  setVelocity       ( float vx, float vy, float vz);
      virtual void getPos            ( float *vx, float *vy, float *vz);
      virtual int  setPos            ( float vx, float vy, float vz);
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

      virtual void getVelocity       ( float *vx, float *vy, float *vz) {_istage->getVelocity(vx,vy,vz);}
      virtual int  setVelocity       ( float vx, float vy, float vz)    {_istage->setVelocity(vx,vy,vz);}
      virtual void getPos            ( float *vx, float *vy, float *vz) {_istage->getPos(vx,vy,vz);}
      virtual int  setPos            ( float vx, float vy, float vz)    {_istage->setPos(vx,vy,vz);}
  };
  // end namespace fetch::Device
}}

