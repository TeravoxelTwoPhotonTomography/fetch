/**
  \file Stage.cpp

  \section impl Stage Implementations

  Right now this is just the C843 controller from physic instruments (http://www.pi-usa.us/index.php)
  and a mock stage.

  Might want to add Thor Labs Cube controller eventually.

  I make a lot of assumptions about having a 3d stage system in the design here which limits some
  generalizability.



  \section runstate Run State Model

  There's two design questions here:  How does this work right now?  And, how should it work?

  Right now, attach() opens the controller for a stage system.  The stage system is assumed to have three axes.
  The axes are initialized on attach, but they may not be fully initialized properly.  There's nothing to
  check if axes are properly initialized on arm, but there probably should be.

  The way it should work is that each axis should be treated as an individual device.  A stage system should be
  an aggregate device.  Each axis should be "attached" when a

  \todo Add an isReady? check or an on_arm call that queries the stage system for whether it is able to respond to
        commands.
  \todo Add stage polling threads that change the device state based on status.
        e.g. If a stage axis goes into an error state it should disarm the associated agent.


  \section err-macros Error handling macros

  There are a few different error handling macros here.  The CHK versions are the same as what I use elsewhere.
  Most often they are used something like this:
  \code
  int f()
  {
    CHKJMP( step1() );
    CHKJMP( step2() );
    CHKJMP( step3() );
    return 1; // handle success
  Error:
    return 0; // handle an error
  }
  \endcode
  If one of the steps() evaluates as false, a warning message will be logged and execution will move to the "Error" label
  so that the error can be handled.

  The WRN and ERR versions don't jump to an Error label.  Instead they use the applications warning() and error() functions
  to log a messge.  Since error() handles fatal errors, it will cause the microscope to shutdown.

  The C843 versions have essentially the same behavior but they use a C843 API error handler to interpret return codes from
  C843 API functions.  These macros also help with thread safely.  Every C843 call needs to be called from within the C843 mutex.
  Since most C843 API calls need to have error checking, these macros also handle locking and unlocking the mutex, even in the
  case of an exception.

  There are two versions of the C843 macros.  One set is for calling inside a C843Stage member function.  The other set is for calling
  in non-member functions.  See the macro's documentation for more info.


  \date   Apr 19, 2010
  \author Nathan Clack <clackn@janelia.hhmi.org>
*/
/**
  \copyright 2010 Howard Hughes Medical Institute.
  All rights reserved.
  Use is subject to Janelia Farm Research Campus Software Copyright 1.1
  license terms (http://license.janelia.org/license/jfrc_copyright_1_1.html).
*/

#include <iostream>

#include <list>
#include <string>
#include <sstream>

#include "stage.h"
#include "task.h"
#include "tiling.h"
#include "thread.h"

#ifdef HAVE_C843
#include "C843_GCS_DLL.H"
#endif


namespace fetch  {

  bool operator==(const cfg::device::C843StageController& a, const cfg::device::C843StageController& b) {return equals(&a,&b);}
  bool operator==(const cfg::device::SimulatedStage& a, const cfg::device::SimulatedStage& b)           {return equals(&a,&b);}
  bool operator==(const cfg::device::Stage& a, const cfg::device::Stage& b)                             {return equals(&a,&b);}

  bool operator!=(const cfg::device::C843StageController& a, const cfg::device::C843StageController& b) {return !(a==b);}
  bool operator!=(const cfg::device::SimulatedStage& a, const cfg::device::SimulatedStage& b)           {return !(a==b);}
  bool operator!=(const cfg::device::Stage& a, const cfg::device::Stage& b)                             {return !(a==b);}

namespace device {


  //
  // C843Stage
  //
/**
  \def C843WRN(expr)
  Evaluates expr from within the C843 mutex and interprets C843 API errors.  If there's a problem, a warning message is logged.
  \def C843ERR(expr)
  Evaluates expr from within the C843 mutex and interprets C843 API errors.  If there's a problem, an error message is logged.  Will cause a shutdown on error.
  \def C843JMP(expr)
  Evaluates expr from within the C843 mutex and interprets C843 API errors.  If there's a problem, a warning message is logged, and execution jumps to an Error label.
  which must be defined somewhere in the calling function's scope.

  \def C843WRN2(expr)
  Same as \ref C843WRN but for use outside of a C843Stage member function.  A valid C843Stage *self must be in scope.
  \def C843ERR2(expr)
  Same as \ref C843ERR but for use outside of a C843Stage member function.  A valid C843Stage *self must be in scope.
  \def C843JMP2(expr)
  Same as \ref C843JMP but for use outside of a C843Stage member function.  A valid C843Stage *self must be in scope.
*/
#ifdef HAVE_C843
#define C843WRN( expr )  {BOOL _v; lock_(); _v=(expr); unlock_(); (c843_error_handler( handle_, _v, #expr, __FILE__, __LINE__, warning ));}
#define C843ERR( expr )  {BOOL _v; lock_(); _v=(expr); unlock_(); (c843_error_handler( handle_, _v, #expr, __FILE__, __LINE__, error   ));}
#define C843JMP( expr )  {BOOL _v; lock_(); _v=(expr); unlock_(); if(c843_error_handler( handle_, _v, #expr, __FILE__, __LINE__, warning)) goto Error;}
#define C843JMPSILENT( expr )  {BOOL _v; lock_(); _v=(expr); unlock_(); if(c843_error_handler( handle_, _v, #expr, __FILE__, __LINE__, NULL)) goto Error;}

#define C843WRN2( expr )  {BOOL _v; self->lock_(); _v=(expr); self->unlock_(); (c843_error_handler( self->handle_, _v, #expr, __FILE__, __LINE__, warning ));}
#define C843ERR2( expr )  {BOOL _v; self->lock_(); _v=(expr); self->unlock_(); (c843_error_handler( self->handle_, _v, #expr, __FILE__, __LINE__, error   ));}
#define C843JMP2( expr )  {BOOL _v; self->lock_(); _v=(expr); self->unlock_(); if(c843_error_handler( self->handle_, _v, #expr, __FILE__, __LINE__, warning)) goto Error;}

#define C843WRAP( expr )  expr
#else
#define NOTINCLUDED(expr) error("(%s:%d) Software was configured without C843 controller support."ENDL "\t%s"ENDL,__FILE__,__LINE__,#expr)
#define C843WRN( expr )   NOTINCLUDED(expr)
#define C843ERR( expr )   NOTINCLUDED(expr)
#define C843JMP( expr )   NOTINCLUDED(expr)
#define C843JMPSILENT( expr )  NOTINCLUDED(expr)

#define C843WRN2( expr )  NOTINCLUDED(expr)
#define C843ERR2( expr )  NOTINCLUDED(expr)
#define C843JMP2( expr )  NOTINCLUDED(expr)

#define C843WRAP( expr )  NOTINCLUDED(expr)
#endif

#define  CHKJMP( expr )  if(!(expr)) {warning("C843:"ENDL "%s(%d): %s"ENDL "Expression evaluated as false"ENDL,__FILE__,__LINE__,#expr); goto Error;}
#define  CHKWRN( expr )  if(!(expr)) {warning("C843:"ENDL "%s(%d): %s"ENDL "Expression evaluated as false"ENDL,__FILE__,__LINE__,#expr); }
#define  CHKERR( expr )  if(!(expr)) {error  ("C843:"ENDL "%s(%d): %s"ENDL "Expression evaluated as false"ENDL,__FILE__,__LINE__,#expr); goto Error;}

#define WARN(msg) warning("Stage:"ENDL "\t%s(%d)"ENDL "\t%s"ENDL,__FILE__,__LINE__,msg)
#define PANIC(e)  if(!(e)) {error  ("C843:"ENDL "%s(%d): %s"ENDL "Expression evaluated as false"ENDL,__FILE__,__LINE__,#e);}

  /// Interprets return codes from C843 API calls.
  /// Usually called from a macro.  See, for example, \ref C843JMP.
  /// \param[in] handle The board id returned by the C843 API when the board was opened.
  /// \param[in] ok     The value of a test expression.  False (or 0) indicates a problem.
  /// \param[in] expr   A string reflecting the test expression.
  /// \param[in] file   Name of the source file where the error was generated.
  /// \param[in] line   The line number where the error was generated.
  /// \param[in] report The reporter function to use e.g. debug(), warning(), or error().  May be NULL, in which case no message will be output.
  /// \returns 0 if no error, otherwise 1
  static
  bool c843_error_handler(long handle, BOOL ok, const char* expr, const char* file, const int line, pf_reporter report)
  {
  #ifdef HAVE_C843
    char buf[1024];
    long e;
    if(handle<0)
    { if(report) report(
        "(%s:%d) C843 Error:"ENDL
        "\t%s"ENDL
        "\tInvalid device id (Got %d)."ENDL,
        file,line,expr,handle);
      return true;
    }
    else if(!ok)
    { e = C843_GetError(handle);

      if(C843_TranslateError(e,buf,sizeof(buf)))
      {
        if(report) report(
          "(%s:%d) C843 Error %d:"ENDL
          "\t%s"ENDL
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
#else
      error("(%s:%d) Software was configured without C843 controller support."ENDL,
            "\t%s"ENDL
            __FILE__,__LINE__,expr);
#endif
  }

#pragma warning(push)
#pragma warning(disable:4244) // lots of float <-- double conversions

  static const char c843_stage_position_log_file[] =  "../stage_position.f64";  ///< The polled stage position gets cached here.

  /** Writes current position and velocity to a cache file with a time since last log.

      The velocity and time can be used to estimate an upper bound to the logged stage position.
      \returns 1 on success, 0 otherwise
  */
  static
  int log_(double r[3], double v[3], double dt)
  { FILE *fp = NULL;
    CHKJMP( fp=fopen(c843_stage_position_log_file,"wb"));
    fwrite(r  ,sizeof(double),3,fp);
    fwrite(v  ,sizeof(double),3,fp);
    fwrite(&dt,sizeof(double),1,fp);
    fclose(fp);
    return 1;
Error:
    return 0;
  }

  /** Tries to read position from log file.
      \param[out] r   Last recorded stage position
      \param[in]  tol Tolerance for uncertainty of last recorded position.
                      This is used to determine whether the recorded position is trustworthy.
      \returns 1 on success (valid position), 0 otherwise (error or position not trusted)
  */
  static
  int maybe_read_position_from_log(double r[3], double tol)
  {
    FILE *fp = NULL;
    double v[3],dt;
    CHKJMP(fp=fopen(c843_stage_position_log_file,"rb"));
    fread(r  ,sizeof(double),3,fp);
    fread(v  ,sizeof(double),3,fp);
    fread(&dt,sizeof(double),1,fp);
    fclose(fp);
    return (fabs(v[0]*dt)<tol)
         &&(fabs(v[1]*dt)<tol)
         &&(fabs(v[2]*dt)<tol);
  Error:
    return 0;
  }

  /** Stage position polling/logging thread.

      \param[in] self_ A pointer to a C843Stage instance.

      The thread should be started after the stage is attached.
      The thread will stop when the position query fails, presumably because the stage's handle
      was closed.

      The cached file gets written in the working directory.
      It records a time between position queries,the stage velocity and the current position.
  */
  static
  void *poll_and_cache_stage_position(void* self_)
  { C843Stage *self = (C843Stage*)self_;
    double r[3]={0},v[3]={0},dt=0.0;
    TicTocTimer clock = tic();
    while(1)
    { long ready;
      Sleep(10);
      C843JMP2(C843_IsControllerReady(self->handle_,&ready));
      if(ready)
      { C843JMP2(C843_qPOS(self->handle_,"123",r));
        C843JMP2(C843_qVEL(self->handle_,"123",v));
        dt = toc(&clock);
        if(!log_(r,v,dt))
        { warning("%s(%d): C843 Failed to write to position log."ENDL,__FILE__,__LINE__);
          goto Error;
        }
      }
    }
Error:
    debug("%s(%d): "ENDL "\t!!!! STAGE POSITION POLLING THREAD GOING DOWN !!!!"ENDL,__FILE__,__LINE__);
    return NULL;
  }

  C843Stage::C843Stage( Agent *agent )
   :StageBase<cfg::device::C843StageController>(agent)
   ,handle_(-1)
   ,logger_(0)
  {
    InitializeCriticalSection(&c843_lock_);
  }

  C843Stage::C843Stage( Agent *agent, Config *cfg )
   :StageBase<cfg::device::C843StageController>(agent,cfg)
   ,handle_(-1)
   ,logger_(0)
  {
    InitializeCriticalSection(&c843_lock_);
  }

  C843Stage::~C843Stage()
  { DeleteCriticalSection(&c843_lock_);
  }

  void C843Stage::lock_()
  { EnterCriticalSection(&c843_lock_);
  }

  void C843Stage::unlock_()
  { LeaveCriticalSection(&c843_lock_);
  }


  /** The attach callback for this device.
      Connects to the controller, and tries to load axes.  Attempts to set the last known stage position, so the stage is
      ready for absolute moves.
      \return 0 on success, 1 otherwise
      \todo Should do some validation.  Make sure stage name is in database.  I suppose CST does this though.
  */
  unsigned int C843Stage::on_attach()
  {
    long ready = 0;
    C843JMP( (handle_=C843_Connect(_config->id()))>=0 );

    #if 0
    // Print known stages
    { char stages[1024*1024];
      if(C843_qVST(handle_,stages,sizeof(stages)))
      { debug("%s\n",stages);
      }
      else
      debug("qVST failed.\n");
    }
    #endif

    for(int i=0;i<_config->axis_size();++i)
    { char id[10];
      const char *name;
      itoa(_config->axis(i).id(),id,10);
      name = _config->axis(i).stage().c_str();
      C843JMP( C843_CST(handle_,id,name) );  //configure selected axes
    }
    C843JMP( C843_INI(handle_,""));          //init all configured axes
    waitForController_();

    { double r[3];
      if(maybe_read_position_from_log(r,0.2/*mm*/))
        CHKWRN(setKnownReference(r[0],r[1],r[2]));
    }
    CHKJMP(logger_=Thread_Alloc(poll_and_cache_stage_position,this));
    return 0; // ok
Error:
    debug("%s(%d): %s()\n"
          "If the stage database could not be found copy the installed database to where the fetch exectuable is located.\n",
          __FILE__,__LINE__,__FUNCTION__);
    return 1; // fail
  }

  // return 0 on success
  unsigned int C843Stage::on_detach()
  {
    int ecode = 0;
    C843JMP( C843_STP(handle_) );  // all stop - HLT is more gentle
    handle_ = -1; //invalid
    Thread_Free(logger_);
Finalize:
    lock_();
    C843WRAP(C843_CloseConnection(handle_)); // always succeeds
    unlock_();
    handle_=-1;                    // set to invalid value.  Required for position logging thread to exit.
    return ecode;
Error:
    ecode = 1;
    goto Finalize;
  }

  static int is_in_bounds(StageTravel *t, float x, float y, float z)
  { return((t->x.min<=x) && (x<=t->x.max)
         &&(t->y.min<=y) && (y<=t->y.max)
         &&(t->z.min<=z) && (z<=t->z.max));
  }
  int C843Stage::getTravel(StageTravel* out)
  {
    /* NOTES:
       o  qTMN and q TMX I think
       o  There's a section in the manual on travel range adjustment (section 5.4)
       o  Assume that (xyz) <==> "123"
       */
    double t[3];
    C843JMP( C843_qTMN(handle_,"123",t) );

    out->x.min = t[0];
    out->y.min = t[1];
    out->z.min = t[2];

    C843JMP( C843_qTMX(handle_,"123",t) );
    out->x.max = t[0];
    out->y.max = t[1];
    out->z.max = t[2];
    return 1;
Error:
    return 0;
  }

  int C843Stage::getVelocity( float *vx, float *vy, float *vz )
  { double t[3];
    C843JMP( C843_qVEL(handle_,"123",t) );
    *vx = t[0];
    *vy = t[1];
    *vz = t[2];
    return 1;
Error:
    return 0;
  }

  int C843Stage::setVelocity( float vx, float vy, float vz )
  { const double t[3] = {vx,vy,vz};
    C843JMP( C843_VEL(handle_,"123",t) );
    return 1; // success
Error:
    return 0;
  }

  void C843Stage::setVelocityNoWait( float vx, float vy, float vz )
  { setVelocity(vx,vy,vz); // doesn't block
  }

  int C843Stage::getPos( float *x, float *y, float *z )
  { double t[3];
    C843JMPSILENT( C843_qPOS(handle_,"123",t) );
    *x = t[0];
    *y = t[1];
    *z = t[2];
    return 1;
Error:
    return 0;
  }

  int C843Stage::getTarget( float *x, float *y, float *z )
  { double t[3];
    C843JMPSILENT( C843_qMOV(handle_,"123",t) );
    *x = t[0];
    *y = t[1];
    *z = t[2];
    return 1;
Error:
    return 0;
  }

  static BOOL all(BOOL* bs, int n)
  { while(n--) if(!bs[n]) return 0;
    return 1;
  }
  static BOOL any(BOOL *bs, int n)
  { BOOL ret = 0;
    while(n--) ret |= bs[n];
    return ret;
  }
  static int same(double *a, double *b, int n)
  { while(n-->0)
      if( fabs(a[n]-b[n])>1e-4 /*mm*/ )
        return 0;
    return 1;
  }


/** Moves the stage to the indicated position.  Will not return till finished (or error).
  \todo  better error handling in case I hit limits
  \todo  what is behavior around limits?
*/
  int C843Stage::setPos( float x, float y, float z, int sleep_ms/*=500*/)
  { double t[3] = {x,y,z};
    BOOL ontarget[] = {0,0,0};
    { StageTravel t;
      CHKJMP(getTravel(&t));
      CHKJMP(is_in_bounds(&t,x,y,z));
    }

    { float vx,vy,vz;
      getVelocity(&vx,&vy,&vz);
      //debug("(%s:%d): C843 Velocity %f %f %f"ENDL,__FILE__,__LINE__,vx,vy,vz);
    }
    C843JMP( C843_HLT(handle_,"123") );              // Stop any motion in progress
    C843JMP( C843_MOV(handle_,"123",t) );            // Move!
    waitForMove_();                                  // Block here until not moving (or error)

    while(!all(ontarget,3))
    { C843JMP( C843_qONT(handle_,"123",ontarget) );    // Ensure controller is on-target (if there was an error before, will repeat here?)
      Sleep(20);  
    }
    if(sleep_ms>0)
      Sleep(sleep_ms); // let the bath soln settle. commented by Vadim on 6/15/13

    { double a[3];
      C843JMP( C843_qMOV(handle_,"123",a) );
      if(!same(a,t,3))
      { double b[3]={0.0,0.0,0.0};
        lock_();
        C843WRAP(C843_qPOS(handle_,"123",b));
        unlock_();
        warning(
          "%s(%d): C843 Move command was interupted for a new destination."ENDL
          "\t  Original Target: %f %f %f"ENDL
          "\t   Current Target: %f %f %f"ENDL
          "\t Current Position: %f %f %f"ENDL,
          __FILE__,__LINE__,
          t[0],t[1],t[2],
          a[0],a[1],a[2],
          b[0],b[1],b[2]);
        goto Error;
      }
    }
    return 1; // success
Error:
    return 0;
  }

  void C843Stage::setPosNoWait( float x, float y, float z )
  { double t[3] = {x,y,z};

    //{ float vx,vy,vz;
    //  getVelocity(&vx,&vy,&vz);
    //  debug("(%s:%d): C843 Velocity %f %f %f"ENDL,__FILE__,__LINE__,vx,vy,vz);
    //}
    C843JMP( C843_HLT(handle_,"123") );              // Stop any motion in progress
    C843JMP( C843_MOV(handle_,"123",t) );            // Move!
Error:
    return;
  }

  bool C843Stage::isMoving()
  { BOOL b;
    C843JMPSILENT( C843_IsMoving(handle_,"",&b) );
    return b;
Error:
    return false; // if there's an error state, it's probably not moving
  }


  bool C843Stage::isOnTarget()
  { BOOL b[] = {0,0,0};
    C843JMPSILENT( C843_qONT(handle_,"123",b) );
    if(all(b,3))
      return true;
Error:
    return false; // if there's an error state, it's probably not on target
  }

  bool C843Stage::isServoOn()
  { BOOL b[] = {0,0,0};
    C843JMPSILENT( C843_qSVO(handle_,"123",b) );
    if(all(b,3))
      return true;
Error:
    return false; // if there's an error state, it's probably not on
  }

  bool C843Stage::clear()
  { C843JMP( C843_CLR(handle_,"123") );
    return true;
Error:
    return false; // if there's an error state, it's probably not on
  }

  bool C843Stage::prepareForCut( unsigned axis)
  { const long pid[]={0x1,0x2,0x3};
    double vals[3]={0};
    CHKJMP(axis==0 || axis==1); // x or y
    { const char a=axis?'2':'1';
      const char axiscode[]={a,a,a,0};
      C843JMP(C843_qSPA(handle_,axiscode,pid,vals,NULL,0));
      P_=vals[0];
      I_=vals[1];
      D_=vals[2];
      vals[0]=_config->cut_proportional_gain();
      vals[1]=_config->cut_integration_gain();
      vals[2]=_config->cut_derivative_gain();
      C843JMP(C843_SPA(handle_,axiscode,pid,vals,NULL));
      cut_mode_active_=true;
    }
    return true;
  Error:
    cut_mode_active_=false;
    return false;
  }
  bool C843Stage::doneWithCut  ( unsigned axis)
  { const long pid[]={0x1,0x2,0x3};
    double vals[3]={0};
    CHKJMP(axis==0 || axis==1); // x or y
    { const char a=axis?'2':'1';
      const char axiscode[]={a,a,a,0};
      vals[0]=P_;
      vals[1]=I_;
      vals[2]=D_;
      C843JMP(C843_SPA(handle_,axiscode,pid,vals,NULL));
    }
    cut_mode_active_=false;
    return true;
  Error:
    return false;
  }

  void C843Stage::waitForController_()
  {
    long ready = 0;
    while(!ready)
    { C843ERR( C843_IsControllerReady(handle_,&ready) );
      Sleep(20); // check ~ 50x/sec
    }
  }

  void C843Stage::waitForMove_()
  {
    BOOL isMoving = TRUE;
    while(isMoving == TRUE)
    { C843JMP( C843_IsMoving(handle_,"",&isMoving) );
      Sleep(20); // check ~ 50x/sec
    }
Error:
    return;
  }

  bool C843Stage::setRefMode_(bool ison)
  { BOOL ons[3] = {ison,ison,ison};
    C843JMP(C843_RON(handle_,"123",ons));
    return 1;
Error:
    return 0;
  }

  bool C843Stage::isReferenced(bool *isok/*=NULL*/)
  { BOOL isrefd[3] ={0,0,0};
    if(isok) *isok=1;
    C843JMPSILENT(C843_IsReferenceOK(handle_,"123",isrefd));
    return all(isrefd,3);
Error:
    if(isok) *isok=0;
    return false;
  }

  bool C843Stage::reference()
  { bool isok=1;
    CHKJMP(setRefMode_(1));
    C843JMP( C843_FRF(handle_,"123") );  //fast reference
    waitForController_();
    if(isReferenced(&isok) && isok)
      return true;
Error:
    warning("%s(%d)"ENDL "\tReferencing failed for one or more axes."ENDL,__FILE__,__LINE__);
    return false;
  }

  void C843Stage::referenceNoWait()
  { bool isok=1;
    CHKJMP(setRefMode_(1));
    C843JMP( C843_FRF(handle_,"123") );  //fast reference
    return;
Error:
    warning("%s(%d)"ENDL "\tReferencing failed for one or more axes."ENDL,__FILE__,__LINE__);
    return;
  }

  bool C843Stage::setKnownReference(float x, float y, float z)
  { double r[3] = {x,y,z};
    bool isok;
    if(isReferenced(&isok))  return true;  // ignore if referenced
    CHKJMP(isok);
    CHKJMP(setRefMode_(0));
    C843JMP(C843_POS(handle_,"123",r));
    if(isReferenced(&isok) && isok)
      return true;
Error:
    warning("%s(%d)"ENDL "setKnownReference failed for one or more axes."ENDL,__FILE__,__LINE__);
    return false;
  }
#pragma warning(pop)


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

  int SimulatedStage::getTravel( StageTravel* out )
  {
    Config c = get_config();
    if(c.axis_size()<3)
    {
      memset(out,0,sizeof(StageTravel));
      return 1;
    }

    out->x.min = c.axis(0).min_mm();
    out->x.max = c.axis(0).max_mm();
    out->y.min = c.axis(1).min_mm();
    out->y.max = c.axis(1).max_mm();
    out->z.min = c.axis(2).min_mm();
    out->z.max = c.axis(2).max_mm();
    return 1;
  }

  int SimulatedStage::getVelocity( float *vx, float *vy, float *vz )
  { *vx=vx_;
    *vy=vy_;
    *vz=vz_;
    return 1;
  }

  int SimulatedStage::setVelocity( float vx, float vy, float vz )
  { vx_=vx;vy_=vy;vz_=vz;
    return 1;
  }

  int SimulatedStage::getPos( float *x, float *y, float *z )
  { *x=x_;
    *y=y_;
    *z=z_;
    return 1;
  }

  static int is_in_bounds(const SimulatedStage::Config& cfg, int iaxis, float v)
  { return (cfg.axis(iaxis).min_mm()<=v) && (v<=cfg.axis(iaxis).max_mm());
  }

  int SimulatedStage::setPos( float x, float y, float z,int sleep_ms )
  {
    if( is_in_bounds(*_config,0,x)
      &&is_in_bounds(*_config,1,y)
      &&is_in_bounds(*_config,2,z))
    { x_=x;y_=y;z_=z;
    } else
    { WARN("Position out of bounds.");
      return 0;
    }
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
    ,_tiling_lock(NULL)
  {
    PANIC(_tiling_lock=Mutex_Alloc());
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
    ,_tiling_lock(NULL)
  {
    PANIC(_tiling_lock=Mutex_Alloc());
    setKind(_config->kind());
  }

  Stage::~Stage()
  { Mutex_Free(_tiling_lock);
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

  void Stage::setFOV(FieldOfViewGeometry *fov)
  {
    if(!(_fov && _lastfov==(*fov)))
    {
      _fov=fov;
      _lastfov=*fov;
      _createTiling();
    }
    _notifyFOVGeometryChanged();
  }

#define _IN(L,X,H) ( ((L)<=(X)) && ((X)<=(H)) )
#define INAXIS(NAME) _IN(t.NAME.min,NAME,t.NAME.max)
  unsigned int Stage::isPosValid( float x, float  y, float z)
  {
    StageTravel t;
    getTravel(&t);
    return INAXIS(x)&&INAXIS(y)&&INAXIS(z);
  }
#undef _IN
#undef INAXIS

  void Stage::_set_config( Config IN *cfg )
  {
    setKind(cfg->kind());
    Guarded_Assert(_c843||_simulated); // at least one device was instanced
    if(_c843)      _c843->_set_config(cfg->mutable_c843());
    if(_simulated) _simulated->_set_config(cfg->mutable_simulated());
    _config = cfg;

    setVelocity(_config->default_velocity_mm_per_sec());
    set_tiling_z_offset_mm(_config->tile_z_offset_mm());
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
      _config->CopyFrom(cfg);
      set_tiling_z_offset_mm(cfg.tile_z_offset_mm());
  }

  float myroundf(float x) {return floorf(x+0.5f);}
  Vector3z Stage::getPosInLattice()
  { StageTiling::TTransform l2s(_tiling->latticeToStageTransform());
    Vector3f r(getTarget()*1000.0); // convert to um
    Vector3z out=Vector3z((l2s.inverse() * r).unaryExpr(std::ptr_fun<float,float>(myroundf)).cast<size_t>());
    std::cout << __FILE__ << "(" << __LINE__ << ")\r\n\t" << out << std::endl;
    return out;
  }

  float Stage::tiling_z_offset_mm()
  { return _tiling->z_offset_mm();
  }

  void Stage::set_tiling_z_offset_mm(float dz_mm)
  {
    Mutex_Lock(_tiling_lock);
    _tiling->set_z_offset_mm(dz_mm);
    Mutex_Unlock(_tiling_lock);
    _config->set_tile_z_offset_mm(_tiling->z_offset_mm());
    _notifyTilingChanged();
  }


  void Stage::inc_tiling_z_offset_mm(float dz_mm)
  {
    Mutex_Lock(_tiling_lock);
    _tiling->inc_z_offset_mm(dz_mm);
    Mutex_Unlock(_tiling_lock);
    _config->set_tile_z_offset_mm(_tiling->z_offset_mm());
    _notifyTilingChanged();
  }


  // Only use when attached but disarmed.
  void Stage::_createTiling()
  { device::StageTravel travel;
    if(!getTravel(&travel))
      return;
    if(_fov)                                                      // A FOV object is stored by the microscope and in the tiling.  The microscope's should be the reference
    {                                                             // not sure why I made this two distinct objects...
      FieldOfViewGeometry fov = *_fov;                            // this should always point to the microscope's FOV object (not the tiling's)
      _destroyTiling();                                           // this call will invalidate the _fov pointer :(  bad design [??? 2011-11 this comment seems questionable]

      Mutex_Lock(_tiling_lock);
      _tiling = new StageTiling(travel, fov, _config->tilemode());
      Mutex_Unlock(_tiling_lock);
      { TListeners::iterator i;
        for(i=_listeners.begin();i!=_listeners.end();++i)
          _tiling->addListener(*i);
      }
      _notifyTilingChanged();
    }
  }

  void Stage::_destroyTiling()
  { if(_tiling)
    {
      Mutex_Lock(_tiling_lock);
      StageTiling *t = _tiling;
      _tiling = NULL;
      Mutex_Unlock(_tiling_lock);
      _notifyTilingChanged();
      delete t;
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
      (*i)->tiling_changed();
  }

  void Stage::_notifyMoved()
  { TListeners::iterator i;
    for(i=_listeners.begin();i!=_listeners.end();++i)
      (*i)->moved();
  }

  void Stage::_notifyReferenced()
  { TListeners::iterator i;
    for(i=_listeners.begin();i!=_listeners.end();++i)
      (*i)->referenced();
  }

  void Stage::_notiveVelocityChanged()
  { TListeners::iterator i;
    for(i=_listeners.begin();i!=_listeners.end();++i)
      (*i)->velocityChanged();
  }

  void Stage::_notifyFOVGeometryChanged()
  {
    TListeners::iterator i;
    for(i=_listeners.begin();i!=_listeners.end();++i)
      (*i)->fov_changed(_fov);
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
      _destroyTiling();
    return eflag;
  }

  int  Stage::setPos(float  x,float  y,float  z,int sleep_ms)
  {
    int out = _istage->setPos(x,y,z);
    _config->mutable_last_target_mm()->set_x(x);
    _config->mutable_last_target_mm()->set_y(y);
    _config->mutable_last_target_mm()->set_z(z);
    _notifyMoved();
    return out;
  }

  void Stage::setPosNoWait(float  x,float  y,float  z)
  {
#if 0
    Config c = get_config();                         // This block performs a "safe" commit
    c.mutable_last_target_mm()->set_x(x);            // of the last position, but will try to
    c.mutable_last_target_mm()->set_y(y);            // stop/restart runnign tasks.
    c.mutable_last_target_mm()->set_z(z);            //
    set_config_nowait(c);                            //
#else                                                //
    // directly set config                           //
    _config->mutable_last_target_mm()->set_x(x);     // This is not thread-safe.  Multiple threads
    _config->mutable_last_target_mm()->set_y(y);     // calling this at the same time might write
    _config->mutable_last_target_mm()->set_z(z);     // inconsistent values, but that's not really a risk here.
#endif                                               // Better to avoid unneccesarily stopping a running task.

    _istage->setPosNoWait(x,y,z);
    _notifyMoved();
  }
}} // end anmespace fetch::device
