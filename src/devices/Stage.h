#pragma once
#include <stdint.h>
#include "object.h"
#include "stage.pb.h"
#include "agent.h"
#include <list>
#include <set>
#include "devices/FieldOfViewGeometry.h"
#include "thread.h"
#include <Eigen/Core>
using namespace Eigen;

// Forward declarations
typedef void Thread;

// The real deal
#define TODO_ERR   error("%s(%d) TODO"ENDL,__FILE__,__LINE__)
#define TODO_WRN warning("%s(%d) TODO"ENDL,__FILE__,__LINE__)

namespace fetch {

  bool operator==(const cfg::device::C843StageController& a, const cfg::device::C843StageController& b);
  bool operator==(const cfg::device::SimulatedStage& a, const cfg::device::SimulatedStage& b);
  bool operator==(const cfg::device::Stage& a, const cfg::device::Stage& b);

  bool operator!=(const cfg::device::C843StageController& a, const cfg::device::C843StageController& b);
  bool operator!=(const cfg::device::SimulatedStage& a, const cfg::device::SimulatedStage& b);
  bool operator!=(const cfg::device::Stage& a, const cfg::device::Stage& b);

namespace device {

  class StageTiling;
  typedef Matrix<size_t,1,3>           Vector3z;

  struct StageAxisTravel  { float min,max; };
  struct StageTravel      { StageAxisTravel x,y,z; };




  //////////////////////////////////////////////////////////////////////
  ///  Stage
  //////////////////////////////////////////////////////////////////////
  struct TilePos
  { float x,y,z;
  };

  class IStage
  {
    public:
      virtual int  getTravel         ( StageTravel* out)                = 0;
      virtual int  getVelocity       ( float *vx, float *vy, float *vz) = 0;
      virtual int  setVelocity       ( float vx, float vy, float vz)    = 0;
      inline  int  setVelocity       ( const Vector3f &r)                   {return setVelocity(r[0],r[1],r[2]);}
      inline  int  setVelocity       ( float v )                            {return setVelocity(v,v,v); }
      virtual void setVelocityNoWait ( float vx, float vy, float vz)    = 0;
      inline  void setVelocityNoWait ( const Vector3f &r)                   {setVelocityNoWait(r[0],r[1],r[2]);}
      inline  void setVelocityNoWait ( float v )                            {setVelocityNoWait(v,v,v); }
      virtual int  getPos            ( float *x, float *y, float *z)    = 0;
      virtual int  getTarget         ( float *x, float *y, float *z)    = 0;
      inline  Vector3f getPos        ()                                     {float x,y,z; getPos(&x,&y,&z); return Vector3f(x,y,z);}
      virtual int  setPos            ( float  x, float  y, float  z,int sleep_ms=500)    = 0;
      virtual int  setPos            ( const Vector3f &r,int sleep_ms=500)  {return setPos(r[0],r[1],r[2],sleep_ms);}
      virtual int  setPos            ( const TilePos &r,int sleep_ms=500)   {return setPos(r.x,r.y,r.z,sleep_ms);}
      virtual void setPosNoWait      ( float  x, float  y, float  z)    = 0;
      inline  void setPosNoWait      ( const Vector3f &r)                   {setPosNoWait(r[0],r[1],r[2]);}
      virtual bool isMoving          () = 0;
      virtual bool isOnTarget        () = 0;
      virtual bool isServoOn         () = 0;

      virtual bool clear             () = 0;                                ///< Clear error status. \returns 1 on success, 0 otherwise

      virtual bool isReferenced      (bool *isok=NULL) = 0;                 ///< Indicates whether the stage thinks it knows it's absolute position. \param[out] isok is 0 if there's an error, otherwise 1.
      virtual bool reference         () = 0;                                ///< Call the stages referencing procedure - usually moves to a reference switch.  \returns 1 on success, 0 otherwise.
      virtual void referenceNoWait   () = 0;
      virtual bool setKnownReference ( float x, float y, float z)       = 0;///< Tell the stages they are at a known position given by (x,y,z)      

      virtual bool prepareForCut     ( unsigned axis)=0;                    ///< Ready axis for cutting. \returns true on success, otherwise false.
      virtual bool doneWithCut       ( unsigned axis)=0;                    ///< Return axis to normal. \returns true on success, otherwise false.
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
      virtual ~C843Stage();

      virtual unsigned int on_attach();
      virtual unsigned int on_detach();

      virtual int  getTravel         ( StageTravel* out);
      virtual int  getVelocity       ( float *vx, float *vy, float *vz);
      virtual int  setVelocity       ( float vx, float vy, float vz);
      virtual void setVelocityNoWait ( float vx, float vy, float vz);
      virtual int  getPos            ( float *x, float *y, float *z);
      virtual int  getTarget         ( float *x, float *y, float *z);
      virtual int  setPos            ( float  x, float  y, float  z, int sleep_ms=500);
      virtual void setPosNoWait      ( float  x, float  y, float  z);
      virtual bool isMoving          ();
      virtual bool isOnTarget        ();
      virtual bool isServoOn         ();

      virtual bool isReferenced      (bool *isok=NULL);                                   ///< Indicates whether the stage thinks it knows it's absolute position. \param[out] isok is 0 if there's an error, otherwise 1.
      virtual bool reference         ();                                                  ///< Call the stages referencing procedure.  Moves to a reference switch.  \returns 1 on success, 0 otherwise.
      virtual void referenceNoWait   ();
      virtual bool setKnownReference ( float x, float y, float z);                        ///< Tell the stages they are at a known position given by (x,y,z). \returns 1 on success, 0 otherwise.

      virtual bool prepareForCut     ( unsigned axis);                                    ///< Ready axis for cutting. \returns true on success, otherwise false.
      virtual bool doneWithCut       ( unsigned axis);                                    ///< Return axis to normal. \returns true on success, otherwise false.

      virtual bool clear();

    public: //pseudo-private
     int                handle_;          ///< handle to the controller board.
     CRITICAL_SECTION   c843_lock_;       ///< The C843 library isn't thread safe.  This mutex is needed to keep things from crashing.
     Thread            *logger_;          ///< thread responsible for saving the latest stage position to disk

     bool   cut_mode_active_;
     double P_,I_,D_;


     void lock_();                        ///< locks the mutex for making stage access thread-safe
     void unlock_();                      ///< unlocks the mutex
     void waitForController_();           ///< polls controller to see if it's ready.  Returns when it's ready.
     void waitForMove_();                 ///< polls stage while it's moving.  Returns when not moving.
     bool setRefMode_(bool ison);         ///< Sets reference mode for all axes. \returns 1 on success, 0 otherwise
  };

  class SimulatedStage:public StageBase<cfg::device::SimulatedStage>
  {   float x_,y_,z_,vx_,vy_,vz_;
    public:
      SimulatedStage(Agent *agent);
      SimulatedStage(Agent *agent, Config *cfg);

      virtual unsigned int on_attach() {/**TODO**/return 0;}
      virtual unsigned int on_detach() {/**TODO**/return 0;}

      virtual int  getTravel         ( StageTravel* out);
      virtual int  getVelocity       ( float *vx, float *vy, float *vz);
      virtual int  setVelocity       ( float vx, float vy, float vz);
      virtual void setVelocityNoWait ( float vx, float vy, float vz)       {setVelocity(vx,vy,vz);}
      virtual int  getPos            ( float *x, float *y, float *z);
      virtual int  getTarget         ( float *x, float *y, float *z)       {return getPos(x,y,z);}
      virtual int  setPos            ( float  x, float  y, float  z, int sleep_ms=500);
      virtual void setPosNoWait      ( float  x, float  y, float  z)       {setPos(x,y,z);}
      virtual bool isMoving          () {return 0;}
      virtual bool isOnTarget        () {return 1;}
      virtual bool isReferenced      (bool *isok=NULL)                     {if(isok) *isok=true; return 1;}   ///< Indicates whether the stage thinks it knows it's absolute position.  \param[out] isok is 0 if there's an error, otherwise 1.
      virtual bool isServoOn         ()                                    {return true;}
      virtual bool reference         ()                                    {return 1;}                        ///< Call the stages referencing procedure - usually moves to a reference switch.  \returns 1 on success, 0 otherwise.
      virtual void referenceNoWait   ()                                    {reference();}
      virtual bool setKnownReference ( float x, float y, float z)          {return setPos(x,y,z);}            ///< Tell the stages they are at a known position given by (x,y,z)
      virtual bool clear             ()                                    {return 1;}
      virtual bool prepareForCut     ( unsigned axis)                      {return true;}                     ///< Ready axis for cutting. \returns true on success, otherwise false.
      virtual bool doneWithCut       ( unsigned axis)                      {return true;}                     ///< Return axis to normal. \returns true on success, otherwise false.
  };

  class StageListener;

  /** Polymorphic stage class

      Wraps an implementation of IStage.  This class provides the device interface that other components should use.
      The implementation is chosen based on the "kind" parameter specified by the Config.
  */
  class Stage:public StageBase<cfg::device::Stage>
  {
  public:
    typedef std::list<TilePos> TilePosList;

  private:
    typedef std::set<StageListener*>  TListeners;

    C843Stage           *_c843;
    SimulatedStage      *_simulated;
    IDevice             *_idevice;
    IStage              *_istage;
    StageTiling         *_tiling;
    TListeners           _listeners;
    FieldOfViewGeometry *_fov;
    FieldOfViewGeometry  _lastfov;
    Mutex               *_tiling_lock;

    public:
      Stage(Agent *agent);
      Stage(Agent *agent, Config *cfg);
      virtual ~Stage();

      void setKind(Config::StageType kind);
      void setFOV(FieldOfViewGeometry *fov);

      virtual unsigned int on_attach();
      virtual unsigned int on_detach();
      void _set_config( Config IN *cfg );
      void _set_config( const Config &cfg );

      virtual int  getTravel         ( StageTravel* out)                    {return _istage->getTravel(out);}
      virtual int  getVelocity       ( float *vx, float *vy, float *vz)     {return _istage->getVelocity(vx,vy,vz);}
      virtual int  setVelocity       ( float vx, float vy, float vz)        {return _istage->setVelocity(vx,vy,vz);}
      virtual int  setVelocity       ( float v )                            {return setVelocity(v,v,v);}
      virtual int  setVelocity       ( const cfg::device::Point3d &v)       {return setVelocity(v.x(),v.y(),v.z());}
      virtual void setVelocityNoWait ( float vx, float vy, float vz)        {_istage->setVelocityNoWait(vx,vy,vz);}
      virtual int  getPos            ( float *x, float *y, float *z)        {return _istage->getPos(x,y,z);}
      inline  Vector3f getPos        ()                                     {float x,y,z; getPos(&x,&y,&z); return Vector3f(x,y,z);}
      virtual int  getTarget         ( float *x, float *y, float *z)        {return _istage->getTarget(x,y,z);}
      inline  Vector3f getTarget     ()                                     {float x,y,z; getTarget(&x,&y,&z); return Vector3f(x,y,z);}
      virtual int  setPos            ( float  x, float  y, float  z, int sleep_ms=500);
      virtual void setPosNoWait      ( float  x, float  y, float  z);
      virtual int  setPos            ( const Vector3f &r,int sleep_ms=500)                   {return setPos(r[0],r[1],r[2],sleep_ms);}
      virtual int  setPos            ( const TilePos &r,int sleep_ms=500)                    {return setPos(r.x,r.y,r.z,sleep_ms);}
      virtual int  setPos            ( const TilePosList::iterator &cursor,int sleep_ms=500) {return setPos(*cursor,sleep_ms);}
      virtual bool isMoving          ()                                     {return _istage->isMoving();}
      virtual bool isOnTarget        ()                                     {return _istage->isOnTarget();};
      unsigned int isPosValid        ( float  x, float  y, float  z);
      virtual bool isReferenced      (bool *isok=NULL)                      {return _istage->isReferenced(isok);}                                               ///< Indicates whether the stage thinks it knows it's absolute position. \param[out] isok is 0 if there's an error, otherwise 1.
      virtual bool isServoOn         ()                                     {return _istage->isServoOn();}
      virtual bool reference         ()                                     {bool ok=_istage->reference(); if(ok) _notifyReferenced(); return ok;}              ///< Call the stages referencing procedure - usually moves to a reference switch.  \returns 1 on success, 0 otherwise.
      virtual void referenceNoWait   ()                                     {_istage->referenceNoWait();}
      virtual bool setKnownReference ( float x, float y, float z)           {bool ok=_istage->setKnownReference(x,y,z); if(ok) _notifyReferenced(); return ok;} ///< Tell the stages they are at a known position given by (x,y,z)
      virtual bool clear             ()                                     {return _istage->clear();}
      virtual bool prepareForCut     ( unsigned axis)                       {return _istage->prepareForCut(axis);}           ///< Ready axis for cutting. \returns true on success, otherwise false.
      virtual bool doneWithCut       ( unsigned axis)                       {return _istage->doneWithCut(axis);}             ///< Return axis to normal. \returns true on success, otherwise false.

      Vector3z getPosInLattice();
      float    tiling_z_offset_mm();
      void     set_tiling_z_offset_mm(float dz_mm);
      void     inc_tiling_z_offset_mm(float dz_mm);
      void     getLastTarget         ( float *x, float *y, float *z)        { cfg::device::Point3d r=_config->last_target_mm(); *x=r.x();*y=r.y();*z=r.z(); }

              void addListener(StageListener *listener);
              void delListener(StageListener *listener);
      inline  StageTiling* tiling()                                         {return _tiling;}

      /** Only locks if tiling is not NULL */
      inline  StageTiling* tilingLocked()                                   {Mutex_Lock(_tiling_lock); if(!_tiling) Mutex_Unlock(_tiling_lock); return _tiling;}
      inline  void         tilingUnlock()                                   {Mutex_Unlock(_tiling_lock);}
  protected:
      void    _createTiling();       ///< only call when disarmed
      void    _destroyTiling();      ///< only call when disarmed
      void    _notifyTilingChanged();
      void    _notifyMoved();
      void    _notifyReferenced();
      void    _notiveVelocityChanged();
      void    _notifyFOVGeometryChanged();
  };

  //////////////////////////////////////////////////////////////////////
  /// StageListener
  //////////////////////////////////////////////////////////////////////
  ///
  /// Allows other objects to respond to stage events
  /// -  Defines a set of callbacks that will be called for certain events.
  /// -  By default, callbacks do nothing so derived listeners only need
  ///    to overload the callbacks they want to listen to.
  ///
  /// When implementing remember that these are usually called from
  /// a thread running the acquisition process.
  ///
  /// -  don't block!
  ///
  /// \note
  /// Maybe I should use an APC to launch the callback from another thread
  /// so that the acquisition loop is guaranteed not to block.
  /// Not worrying about this right now since stage motion doesn't occur
  /// during super time sensitive parts of the loop.  Still though...
  /// Actually, Qt supports both non-blocking and blocking queued connections
  /// (I'm primarily worried about signal/slot communication with the GUI
  /// frontend here).  Using the QueuedConncection, which is non-blocking,
  /// means the callback shouldn't be blocked.  I can leave the blocking
  /// responsibility up to the callback.

  class StageListener
  {
  public:
    virtual void tiling_changed() {}                                         ///< a new tiling was created.
    virtual void tile_done(size_t index, const Vector3f& pos,uint32_t sts) {}///< the specified tile was marked as done
    virtual void tile_next(size_t index, const Vector3f& pos) {}             ///< the next tile was requested (stage not necessarily moved yet)

    virtual void fov_changed(const FieldOfViewGeometry *fov) {}              ///< the field of view size changed
    virtual void moved() {}                                                  ///< the stage position changed
    virtual void referenced() {}                                             ///< the stage was referenced
    virtual void velocityChanged() {}                                        ///< the velocity set for an axis changed
  };

  // end namespace fetch::Device
}}

