#include "Tiling.h"
#include "Stage.h"

#include <Eigen/Core>
using namespace Eigen;

namespace mylib
{
#include <image.h>
}

#include <iostream>
#include <functional>

//#define DEBUG__WRITE_IMAGES
//#define DEBUG__SHOW

#ifdef DEBUG__SHOW
#define SHOW(e) std::cout << "---" << std::endl << #e << " is " << std::endl << (e) << std::endl << "~~~"  << std::endl;
#else
#define SHOW(e)
#endif

#define PANIC(e) do{if(!(e)) error("%s(%d)"ENDL "\tExpression evaluated to false."ENDL "\t%s"ENDL,__FILE__,__LINE__,#e);}while(0)

/// Simple mutex wrapper for scoped locking/unlocking
class AutoLock
{ Mutex *l_;
public:
  AutoLock(Mutex* l):l_(l) {Mutex_Lock(l_);}
  ~AutoLock() {Mutex_Unlock(l_);}
};
// AutoLock lock(lock_);

namespace fetch {
namespace device {

  typedef uint8_t  uint8;
  typedef uint32_t uint32;

  //////////////////////////////////////////////////////////////////////
  //  StageTiling  /////////////////////////////////////////////////////
  //////////////////////////////////////////////////////////////////////

  //  Constructors  ////////////////////////////////////////////////////
  StageTiling::StageTiling(const device::StageTravel &travel,
                           const FieldOfViewGeometry &fov,
                           const Mode                 alignment)
    :
      attr_(NULL),
      cursor_(0),
      current_plane_offset_(0),
      sz_plane_nelem_(0),
      latticeToStage_(),
      fov_(fov),
      z_offset_um_(0.0),
      travel_(travel),
      lock_(0),
      mode_(alignment)
  {
    PANIC(lock_=Mutex_Alloc());
    computeLatticeToStageTransform_(fov,alignment);
    initAttr_(computeLatticeExtents_(travel_));
    //markAddressable_(&travel_);
  }

  //  Destructor  /////////////////////////////////////////////////////
  StageTiling::~StageTiling()
  {
    if(attr_) Free_Array(attr_);
    if(lock_) Mutex_Free(lock_);
  }

  //  computeLatticeToStageTransform_  /////////////////////////////////
  //
  //  For an index vector i, compute T, such that T*i are the nodes of
  //  the lattice in stage space.
  //
  //  FOV angle should be between 0 and pi/2 (180 degrees).
  void StageTiling::computeLatticeToStageTransform_
                          (const FieldOfViewGeometry &fov,
                           const Mode                 alignment)
  { latticeToStage_ = TTransform::Identity();
    Vector3f sc = fov.field_size_um_ - fov.overlap_um_;
    switch(alignment)
    {
// should probably do the zoffset before rotate/shear but 
// they're orthogonal to z so it doesn't matter.
    case Mode::Stage_TilingMode_PixelAligned:
        // Rotate the lattice
        latticeToStage_
          .rotate( AngleAxis<float>(fov.rotation_radians_,Vector3f::UnitZ()) )
          .translate(Vector3f(0,0,z_offset_um_))
          .scale(sc)
          ;
        SHOW(latticeToStage_.matrix());
        return;
    case Mode::Stage_TilingMode_StageAligned:
        // Shear the lattice
        float th = fov.rotation_radians_;
        latticeToStage_.linear() =
          (Matrix3f() <<
            1.0f/cos(th), -sin(th), 0,
                       0,  cos(th), 0,
                       0,        0, 1).finished();
        latticeToStage_.translate(Vector3f(0,0,z_offset_um_))
                       .scale(sc)
                       ;
        return;
    }
  }

 void StageTiling::set_z_offset_um(f64 z_um)
 { z_offset_um_=z_um;
   computeLatticeToStageTransform_(fov_,mode_);
 }
 void StageTiling::inc_z_offset_um(f64 z_um)
 { z_offset_um_+=z_um;
   computeLatticeToStageTransform_(fov_,mode_);
 }
 f64 StageTiling::z_offset_um() 
 { return z_offset_um_;
 }

  //  computeLatticeExtents_  //////////////////////////////////////////
  //
  //  Find the range of indexes that cover the stage.
  //  This is done by applying the inverse latticeToStage_ transform to the
  //    rectangle described by the stage extents, and then finding extreema.
  //  The latticeToStage_ transfrom is adjusted so the minimal extrema is the
  //    origin.  That is, [min extremal] = T(0,0,0).

  mylib::Coordinate* StageTiling::computeLatticeExtents_(const device::StageTravel& travel)
  {
    Matrix<float,8,3> sabox; // vertices of the cube, stage aligned
    sabox << // travel is in mm
         travel.x.min,   travel.y.min,   travel.z.min,
         travel.x.min,   travel.y.max,   travel.z.min,
         travel.x.max,   travel.y.max,   travel.z.min,
         travel.x.max,   travel.y.min,   travel.z.min,

         travel.x.min,   travel.y.min,   travel.z.max,
         travel.x.min,   travel.y.max,   travel.z.max,
         travel.x.max,   travel.y.max,   travel.z.max,
         travel.x.max,   travel.y.min,   travel.z.max;
    sabox *= 1000.0; //mm to um

    Matrix<float,3,8> labox; // vertices of the cube, lattice aligned
    labox.noalias() = latticeToStage_.inverse() * sabox.transpose();

    Vector3f maxs,mins;
    maxs.noalias() = labox.rowwise().maxCoeff();
    mins.noalias() = labox.rowwise().minCoeff();

    latticeToStage_.translate(mins);
    Vector3z c((maxs-mins).unaryExpr(std::ptr_fun<float,float>(ceil)).cast<size_t>());
    SHOW(sabox);
    SHOW(labox);
    SHOW(mins);
    SHOW(maxs);
    SHOW(c);

    mylib::Coordinate* out = mylib::Coord3(c(2)+1,c(1)+1,c(0)+1); //shape of the lattice
    return out;
  }

  //  initAttr_  ///////////////////////////////////////////////////////
  //

  void StageTiling::initAttr_(mylib::Coordinate *shape)
  { AutoLock lock(lock_);
    attr_ = mylib::Make_Array_With_Shape(
      mylib::PLAIN_KIND,
      mylib::UINT32_TYPE,
      shape); // shape gets free'd here

    memset(attr_->data,0,sizeof(uint32_t)*attr_->size); //unset
    sz_plane_nelem_ = attr_->dims[0] * attr_->dims[1];
  }

  //  resetCursor  /////////////////////////////////////////////////////
  //

#define ON_PLANE(e)    (((e)>=current_plane_offset_) && ((e)<(current_plane_offset_ + sz_plane_nelem_)))
#define ON_LATTICE(e)  ((e) < (attr_->size))

  typedef mylib::Dimn_Type Dimn_Type;
  void StageTiling::resetCursor()
  { AutoLock lock(lock_);
    cursor_ = 0;

    uint32_t* mask     = AUINT32(attr_);
    uint32_t  attrmask = Addressable | Safe | Active | Done,
              attr     = Addressable | Safe | Active;

    while( (mask[cursor_] & attrmask) != attr
        && ON_LATTICE(cursor_) )
    {++cursor_;}

    if(ON_LATTICE(cursor_) &&  (mask[cursor_] & attrmask) == attr)
    { mylib::Coordinate *c = mylib::Idx2CoordA(attr_,cursor_);
      current_plane_offset_ = ADIMN(c)[2] * sz_plane_nelem_;
      mylib::Free_Array(c);
      cursor_ -= 1; // so next*Cursor will be the first tile
    } else
      cursor_=0;
  }

  //  setCursorToPlane  ////////////////////////////////////////////////
  //

  /// \todo bounds checking
  void StageTiling::setCursorToPlane(size_t iplane)
  { AutoLock lock(lock_);
    cursor_=current_plane_offset_=iplane*sz_plane_nelem_;
    cursor_--; // always incremented before query, so subtracting one here means first tile will not be skipped
  }

  //  nextInPlanePosition  /////////////////////////////////////////////
  //

  bool StageTiling::nextInPlanePosition(Vector3f &pos)
  { lock();
    uint32_t* mask = AUINT32(attr_);
    uint32_t attrmask = Addressable | Safe | Active | Done,
             attr     = Addressable | Safe | Active;

    do{++cursor_;}
    while( (mask[cursor_] & attrmask) != attr
        && ON_PLANE(cursor_) );

    if(ON_PLANE(cursor_) &&  (mask[cursor_] & attrmask) == attr)
    { pos = computeCursorPos();
      unlock();
      notifyNext(cursor_,pos);
      return true;
    } else
    { unlock();
      return false;
    }
  }

  //  nextInPlane                    /////////////////////////////////////////////
  //
  bool StageTiling::nextInPlaneQuery(Vector3f &pos,uint32_t attrmask,uint32_t attr)
  { lock();
    uint32_t* mask = AUINT32(attr_);
    do{++cursor_;}
    while( (mask[cursor_] & attrmask) != attr
        && ON_PLANE(cursor_) );

    if(ON_PLANE(cursor_) &&  (mask[cursor_] & attrmask) == attr)
    { pos = computeCursorPos();
      unlock();
      notifyNext(cursor_,pos);
      return true;
    } else
    { unlock();
      return false;
    }
  }


  //  nextInPlaneExplorablePosition  /////////////////////////////////////////////
  //

  /** Return the next tile-position to explore in the current tile-plane.

      Exclude tiles that are not Addressable, or that have already been
      marked Active or Done.
  */

  bool StageTiling::nextInPlaneExplorablePosition(Vector3f &pos)
  { lock();
    uint32_t* mask = AUINT32(attr_);
    uint32_t attrmask = Explorable | Safe | Explored | Addressable | Active | Done,
             attr     = Explorable | Safe | Addressable;

    do{++cursor_;}
    while( (mask[cursor_] & attrmask) != attr
        && ON_PLANE(cursor_) );

    if(ON_PLANE(cursor_) &&  (mask[cursor_] & attrmask) == attr)
    { pos = computeCursorPos();
      unlock();
      notifyNext(cursor_,pos);
      return true;
    } else
    { unlock();
      return false;
    }
  }

  //  nextPosition  ////////////////////////////////////////////////////
  //
  bool StageTiling::nextPosition(Vector3f &pos)
  { lock();
    uint32_t* mask    = AUINT32(attr_);
    uint32_t attrmask = Addressable | Safe | Active | Done,
             attr     = Addressable | Safe | Active;

    do {++cursor_;}
    while( (mask[cursor_] & attrmask) != attr
        && ON_LATTICE(cursor_) );

    if(ON_LATTICE(cursor_))
    { pos = computeCursorPos();
      unlock();
      notifyNext(cursor_,pos);
      return true;
    } else
    { unlock();
      return false;
    }
  }

  //  nextSearchPosition  //////////////////////////////////////////////
  //
  // *ctx should be NULL on the first call.  It will be internally managed.

  struct tile_search_parent_t
  { uint32_t *c,dir,n;
  };

  
    tile_search_history_t::tile_search_history_t():history(0),i(0),n(0){}
    tile_search_history_t::~tile_search_history_t() {if (history) {free(history);}}
    /** Returns 0 on error (memory) */
    int tile_search_history_t::push(uint32_t *c,int dir,int n)
    { if(!request(i+1)) return 0;
      ++i;
      history[i].c  =c;
      history[i].dir=(uint32_t)dir;
      history[i].n  =(uint32_t)n;
      return 1;
    }
    /** Returns 0 on underflow */
    int tile_search_history_t::pop(uint32_t **pc, uint32_t *pdir, uint32_t* pn)
    { if(i<=0) return 0; // zero is never written to, it's reserved for signalling empty
      *pc  =history[i].c;
      *pdir=history[i].dir;
      *pn  =history[i].n;
      --i;
      return 1;
    }
    int tile_search_history_t::pop(uint32_t **pc, int *pdir, int *pn)
    { uint32_t d,n;
      int ret=pop(pc,&d,&n);
      *pdir=d;
      *pn=n;
      return ret;
    }
    /** returns 0 on failure, otherwise 1 */
    int tile_search_history_t::request(size_t  i_)
    { if(i_>=n)
      { size_t oldn=n;
        n=1.2*i_+50;
        history=(tile_search_parent_t*)realloc(history,sizeof(tile_search_parent_t)*n);
        memset(history+oldn,0,sizeof(tile_search_parent_t)*(n-oldn));
      }
      return (history!=NULL);
    }
    void tile_search_history_t::flush() {i=0;}
 
    TileSearchContext::TileSearchContext(StageTiling *t,int radius/*=0*/) : tiling(t), c(0),n(0),dir(0),mode(0),radius(radius) {}
    TileSearchContext::~TileSearchContext()
    { if(tiling) tiling->tileSearchCleanup(this);
    }
    void TileSearchContext::sync()
    {  c    =AUINT32(tiling->attr_)+tiling->cursor_;
       line =tiling->attr_->dims[0];
       plane=tiling->sz_plane_nelem_;
       n=0;
    }
    void TileSearchContext::set_outline_mode(bool tf/*=true*/) { mode=tf; if(!tf) flush();}
    bool TileSearchContext::is_outline_mode() {return mode;}
    bool TileSearchContext::is_hunt_mode()    {return !mode;}
    uint32_t *TileSearchContext::next_neighbor()
    { int steps[]={-1,-line,1,line};
      uint32_t *p=c+steps[ (dir+n)%4 ];
      ++n;
      if(n<4 && tiling->on_plane(p))
        return p;
      return 0;
    }
    void TileSearchContext::flush() {n=dir=0;history.flush();}
    /** returns 0 on failure, otherwise 1 */
    int TileSearchContext::push(uint32_t* p)
    { if(!history.push(c,dir,n))
        return 0;
      c=p;
      tiling->cursor_=c-AUINT32(tiling->attr_);
      dir=(dir+n-2)%4; // remmber n is the next neighbor, subtract 1 for current, subtract another 1 to start with left turn
      n=0;
      return 1;
    }
    int TileSearchContext::pop()
    { if(!history.pop(&c,&dir,&n))
        return false;
      tiling->cursor_=c-AUINT32(tiling->attr_);
      return true;
    }
#define CHECK(e,flag) ((*(e)&(flag))==(flag))
#define MARK(e,flag)  (*(e)|=(flag))
    bool TileSearchContext::detected() {return CHECK(c,StageTiling::Detected); }
    void TileSearchContext::reserve()  { MARK(c,StageTiling::Reserved); }

  bool StageTiling::on_plane(uint32_t *p)
  { return ON_PLANE(p-AUINT32(attr_));
  }

  void StageTiling::tileSearchCleanup(TileSearchContext *ctx)
  { const uint32_t *beg = AUINT32(attr_)+current_plane_offset_,
                   *end = beg+sz_plane_nelem_;
    int iplane=current_plane_offset_/sz_plane_nelem_;
    lock();
    for(uint32_t *t=(uint32_t*)beg;t<end;++t) // Reset Reserved
      *t = t[0]&~Reserved;
    if(ctx->is_outline_mode())
    { unlock(); // FIXME: possible race condition
      fillHoles(iplane,Detected);
      if(ctx->radius)
        dilate(iplane,ctx->radius,Detected,Explorable,0);
      lock();    
      ctx->set_outline_mode(false);
    }
    cursor_=beg-AUINT32(attr_); // reset to start of plane
    // Copy explorable to next plane
    if( (current_plane_offset_/sz_plane_nelem_)<(attr_->dims[2]-1)) // if not last plane
    { uint32_t *c,*n;
      for(c=(uint32_t*)beg,n=(uint32_t*)end;c<end;++c,++n)
        *n|=(*c&Explorable);
    }
    unlock();
  }

#define ELIGABLE(e)          ((*(e)&eligable_mask)==eligable)
#define ALREADY_DETECTED(e)  ((*(e)&(Addressable|Explorable|Detected|Reserved))==(Addressable|Explorable|Detected))
//#define IMPLY_DETECTED(e)    ((*(e)&(Done))==(Done)) | ((*(e)&(Active))==(Active))

  /* Sorry for the goto's.  don't hate. */
  bool StageTiling::nextSearchPosition(int iplane, int radius, Vector3f &pos,TileSearchContext **pctx)
  { const uint32_t eligable_mask = Addressable | Safe | Explorable | Explored /*| Active | Done*/, // Active/Done do not imply detection and we don't want them to
                   eligable      = Addressable | Safe | Explorable,
                  *beg = AUINT32(attr_)+current_plane_offset_,
                  *end = beg+sz_plane_nelem_;
    if(!pctx) return 0;
    TileSearchContext *ctx=*pctx;
    lock();
    if(!ctx)
    { *pctx=ctx=new TileSearchContext(this,radius);
    }
    ctx->sync();
Start:
    if(ctx->detected())
    { ctx->reserve();
      ctx->set_outline_mode();
    }
    if(ctx->is_outline_mode())
    { uint32_t *n;      
      if(ctx->detected())
      { while(n=ctx->next_neighbor())
        { if(CHECK(n,Reserved)) // BOOM DONE - the loop is closed
          { goto DoneOutlining;
          }          
#if 0 // Actually don't want to infer detected from any other flags
          if(IMPLY_DETECTED(n)) //infer_detected - some flags imply tile is detected
            *n|=Detected;
#endif
          if(ALREADY_DETECTED(n)) 
          { ctx->push(n); // treat previously detected as ok part of the perimeter
            goto Start;   // don't need to detect again
          }
          if(ELIGABLE(n))
          { ctx->push(n);
            goto Yield;
          }
        }
      }
      if(ctx->pop())      // Back up.  Also part of handling dead end - no neighbors eligable or reserved
        goto Start;      
      goto DoneOutlining; // underflow...The found region is just a strip, not a closed loop.
    }
Hunt:
    if(ctx->is_hunt_mode())
    { unlock(); // FIXME: racy
      return nextInPlaneQuery(pos,
        Addressable|Safe|Explorable|Explored|Detected|Active|Done,  // skip explored,detected,active or done tiles.
        Addressable|Safe|Explorable                              );
      lock();
    }
    // ALL DONE
    delete ctx;
    *pctx=0;
    unlock();
    return false;
Yield:
    pos=computeCursorPos();
    unlock();
    notifyNext(cursor_,pos);
    return true;
DoneOutlining:
    unlock(); // FIXME - racy
    tileSearchCleanup(ctx); // this cleans up the outlining and resets to hunt mode
    lock();
    goto Hunt;
  }
#undef CHECK
#undef MARK
#undef ELIGABLE

  //  markDone  ////////////////////////////////////////////////////////
  //
  void StageTiling::markDone(bool success)
  { uint32_t *m=0;
    { AutoLock lock(lock_);
      m = AUINT32(attr_) + cursor_;
      *m |= Done;
      if(!success)
        *m |= TileError;
    }
    notifyDone(cursor_,computeCursorPos(),*m);
  }

  //  markSafe  ////////////////////////////////////////////////////////
  //
  void StageTiling::markSafe(bool success)
  { uint32_t *m=0;
    { AutoLock lock(lock_);
      m = AUINT32(attr_) + cursor_;
      *m |= Safe;
      if(!success)
        *m |= TileError;
    }
    notifyDone(cursor_,computeCursorPos(),*m);
  }

  //  markExplored  //////////////////////////////////////////////////////
  //
  void StageTiling::markExplored(bool tf)
  { uint32_t *m=0;
    { AutoLock lock(lock_);
      m = AUINT32(attr_) + cursor_;
      if(tf)
        *m |= Explored;
      else
        *m &= ~Explored;
    }
    notifyDone(cursor_,computeCursorPos(),*m);
  }

  //  markDetected  //////////////////////////////////////////////////////
  //
  void StageTiling::markDetected(bool tf)
  { uint32_t *m=0;
    { AutoLock lock(lock_);
      m = AUINT32(attr_) + cursor_;
      if(tf)
        *m |= Detected;
      else
        *m &= ~Detected;
    }
    notifyDone(cursor_,computeCursorPos(),*m);
  }

  //  markActive  ////////////////////////////////////////////////////////
  //
  void StageTiling::markActive()
  { uint32_t *m=0;
    { AutoLock lock(lock_);
      m = AUINT32(attr_) + cursor_;
      *m |= Active;
    }
    notifyDone(cursor_,computeCursorPos(),*m);
  }

  //  markUserReset  /////////////////////////////////////////////////////
  //
  void StageTiling::markUserReset()
  { uint32_t *m=0;
    { AutoLock lock(lock_);
      m = AUINT32(attr_) + cursor_;
      *m &= ~( Active|Detected|Explored|Explorable|Safe|Done );
    }
    notifyDone(cursor_,computeCursorPos(),*m);
  }

  //  markAddressable  ///////////////////////////////////////////////////
  //
  static inline unsigned in(const Vector3f &a,const Vector3f &b,const Vector3f &v)
  { return (a[0]<v[0]) && (v[0]<b[0])
         &&(a[1]<v[1]) && (v[1]<b[1])
         &&(a[2]<v[2]) && (v[2]<b[2]);
  }
  /** Marks the indicated plane as addressable according to the travel.
  */
  void StageTiling::markAddressable(size_t iplane)
  {
    size_t old = cursor_;
    setCursorToPlane(iplane); // FIXME: chance for another thread to change the cursor...recursive locks not allowed
    AutoLock lock(lock_);
    Vector3f a = Vector3f(travel_.x.min,travel_.y.min,travel_.z.min),
             b = Vector3f(travel_.x.max,travel_.y.max,travel_.z.max);
    uint32_t* v = AUINT32(attr_);
    while(ON_PLANE(++cursor_))
      v[cursor_]|=Addressable*in(a,b,computeCursorPos()*0.001);
    cursor_=old; // restore cursor
  }

  //  anyExplored  ///////////////////////////////////////////////////////////////
  //
  int StageTiling::anyExplored(int iplane)
  {
    setCursorToPlane(iplane);// FIXME: chance for another thread to change the cursor...recursive locks not allowed
    AutoLock lock(lock_);
    uint32_t* mask = AUINT32(attr_);
    uint32_t attrmask = Explorable|Addressable,
             attr     = Explorable|Addressable;

    do{++cursor_;}
    while( (mask[cursor_] & attrmask) != attr
        && ON_PLANE(cursor_) );
    return ON_PLANE(cursor_);
  }

  //  fillHolesInActive  /////////////////////////////////////////////////
  //

  /// Private stack implementation for non-recursive flood fill.
  typedef struct _stack_t
  { size_t     n,cap;
    uint32_t **data;
  } stack_t;
  static void grow(stack_t *s)
  { if(s->n==s->cap-1)
    { size_t newsize = s->cap*1.2+50;
      Guarded_Realloc((void**)&s->data,newsize,"grow stack");
      memset(s->data+s->cap,0,(newsize-s->cap)*sizeof(uint32_t*));
      s->cap=newsize;
    }
  }
  static void push(stack_t *s, uint32_t* v)
  { grow(s);
    s->data[s->n++] = v;
  }
  static uint32_t* pop(stack_t *s)
  { return s->data[--s->n];
  }
  static stack_t make_stack(size_t reserve)
  { stack_t s;
    s.n=0;
    s.data=(uint32_t**)Guarded_Calloc(s.cap=reserve,sizeof(uint32_t*),"make stack");
    return s;
  }
  static void destroy_stack(stack_t *s)
  { if(s && s->data) free(s->data);
  }


  /**
    Looks at tiles in this plane and the previous plane for detection events in
    order to determine whether tiles in the plane should be marked Active

    \returns 1 if any tiles were marked active, otherwise 0.
  */
  int StageTiling::updateActive(size_t iplane)
  { int any=0;
    setCursorToPlane(iplane);// FIXME: chance for another thread to change the cursor...recursive locks not allowed
    AutoLock lock(lock_);
    uint32_t *c,*p,
             *beg  = AUINT32(attr_)+current_plane_offset_,
             *end  = beg+sz_plane_nelem_,
             *prev = beg-sz_plane_nelem_;
    #define DETECTED(e) ((*(e)&Detected)==Detected)
    for(c=beg,p=prev;c<end;c++,p++)
    { if( DETECTED(c) || ((iplane>0)?DETECTED(p):0) )
      { *c |= Active;
        any=1;
      }
    }
    #undef DETECTED
    return any;
  }

  /** Fill holes in regions marked as active

    Filled regions are 4-connected.
  */
  void StageTiling::fillHolesInActive(size_t iplane)
  { fillHoles(iplane,Active);}

  void StageTiling::fillHoles(size_t iplane, StageTiling::Flags flag)
  {
    setCursorToPlane(iplane);// FIXME: chance for another thread to change the cursor...recursive locks not allowed
    AutoLock lock(lock_);
    uint32_t *c,
             *beg = AUINT32(attr_)+current_plane_offset_,
             *end = beg+sz_plane_nelem_;
    int is_open=0;
    const unsigned w=attr_->dims[0],
                   h=attr_->dims[1];
    stack_t stack = make_stack(sz_plane_nelem_);
    for(c=beg;c<end;)
    { uint32 *n,*next;
      uint32 mask = flag | Reserved;
      // mark connected and do bounds test for region
      is_open=0;
      push(&stack,0); // push 0 to detect underflow when the fill is done
      push(&stack,c);
      while(n=pop(&stack))
      {   unsigned x = (n-beg)%w,
                   y = (n-beg)/w;
#if 0
          debug("%3d %3d %15s %15s %15s %15s\n",x,y,
                (n[0]&Reserved)?"Reserved":".",
                (n[0]&flag)?"flag":".",
                (n[0]&Explorable)?"Explorable":".",
                (n[0]&Addressable)?"Addressable":".");
#endif
          *n |= Reserved;
          is_open |= ((x==0)||(x==w-1)||(y==0)||(y==h-1)); // is the region connected to the plane bounds?
          if(!(y>=(h-1) || *(next=(n+w))&mask )) {*next|=Reserved; push(&stack,next);}  // down
          if(!(y<=0     || *(next=(n-w))&mask )) {*next|=Reserved; push(&stack,next);}  // up
          if(!(x<=0     || *(next=(n-1))&mask )) {*next|=Reserved; push(&stack,next);}  // left
          if(!(x>=(w-1) || *(next=(n+1))&mask )) {*next|=Reserved; push(&stack,next);}  // right
      } // end first fill
      if(!is_open)                             // second fill to mark interior as flag
      { mask = flag;                         // edges and self are labeled flag
        push(&stack,0);
        push(&stack,c);
        while(n=pop(&stack))
        {   unsigned x = (n-beg)%w,
                     y = (n-beg)/w;
#if 0
            debug("%3d %3d %15s %15s %15s %15s\n",x,y,
                (n[0]&Reserved)?"Reserved":".",
                (n[0]&flag)?"flag":".",
                (n[0]&Explorable)?"Explorable":".",
                (n[0]&Addressable)?"Addressable":".");
#endif
            *n |= flag;
            if(!(y>=(h-1) || *(next=(n+w))&mask )) {*next|=flag; push(&stack,next);}  // down
            if(!(y<=0     || *(next=(n-w))&mask )) {*next|=flag; push(&stack,next);}  // up
            if(!(x<=0     || *(next=(n-1))&mask )) {*next|=flag; push(&stack,next);}  // left
            if(!(x>=(w-1) || *(next=(n+1))&mask )) {*next|=flag; push(&stack,next);}  // right
        } // end second fill
      }

      while(c++<end && *c&(Reserved|flag));  // move to next unreserved
    } // done searching for regions
    destroy_stack(&stack);
    for(c=beg;c<end;++c)                       // mark all unreserved
      *c = c[0]&~Reserved;
  }

  //  dilateActive  //////////////////////////////////////////////////////
  //
  #define countof(e) (sizeof(e)/sizeof(*e))
  /** Mark tiles as Active if they are 8-connected to an Active tile. */
  void StageTiling::dilateActive(size_t iplane)
  { dilate(iplane,1,Active,Active,1 /*restrict to explorable */);
  }

  // dilate         //////////////////////////////////////////////////////
  //
  // Does not dilate into Done tiles.
  // Optionally restricts dilation to explorable tiles.
  //
  //
  void StageTiling::dilate(size_t iplane, int n, StageTiling::Flags query_flag, StageTiling::Flags write_flag, int explorable_only)
  {
    setCursorToPlane(iplane); // FIXME: chance for another thread to change the cursor...recursive locks not allowed
    AutoLock lock(lock_);
    uint32_t *c,
             *beg = AUINT32(attr_)+current_plane_offset_,
             *end = beg+sz_plane_nelem_;
    const unsigned w=attr_->dims[0],
                   h=attr_->dims[1];
    unsigned       x,y,j;

    const int offsets[] = {-w-1,-w,-w+1,-1,1,w-1,w,w+1};
    const unsigned top=1,left=2,bot=4,right=8; // bit flags
#define MAYBE_EXPLORABLE ((explorable_only)?Explorable:0)
    const unsigned masks[]   = {top|left,top,top|right,left,right,bot|left,bot,bot|right};
    const unsigned attrmask = Reserved|Safe|Addressable|Done|MAYBE_EXPLORABLE,            // attr is the neighbor query
                   attr     = Reserved|Safe|Addressable     |MAYBE_EXPLORABLE,
                   lblmask  = Addressable|MAYBE_EXPLORABLE;                          // lblmask selects for valid write points
#undef MAYBE_EXPLORABLE
    for(c=beg;c<end;++c)                             // mark original active tiles as reserved
      *c |= ((*c&query_flag)==query_flag)*Reserved;  // this way only the originally labelled tiles will be dilated from
    while(n-->0)
    { for(y=0;y<h;++y)
      { unsigned rowmask = ((y==0)*top)|((y==h-1)*bot);
        for(x=0;x<w;++x)
        { const unsigned colmask = ((x==0)*left)|((x==w-1)*right),
                            mask = rowmask|colmask;
          c=beg+y*w+x;
          if((*c&lblmask)==lblmask)                // mark only tile that are addressable and explorable
            for(j=0;j<countof(offsets);++j)
              if(  (mask&masks[j])==0              // is neighbor in bounds
                && (c[offsets[j]]&attrmask)==attr) // query neighbor attribute for match
              { *c|=(write_flag|Reserved2); break;
              }
        }
      }
      for(c=beg;c<end;++c)                     // make query transitive over dilation radius
        if(*c&Reserved2)
        { *c|=Reserved;
          *c = c[0]&~Reserved2;
        }
    }
    for(c=beg;c<end;++c)                       // mark all unreserved
      *c = c[0]&~Reserved;
  }


  // minDistTo ///////////////////////////////////////////////////////////////
  //
  // returns -1 on error, otherwise
  //         the lattice distance from the current tile to the nearest tile 
  //         in plane satisfying the query.
  //
  // Algorithm is breadth first search.  So it requires a FIFO queue.
  //
  // NOT THREAD SAFE
  // - right now this is only called by AdaptiveTilingTask
  // - to make it thread safe
  //   - make estore and friends non-static
  //   or
  //   - protext estore and friends
  //   - deallocation has to track which elements are used and which are free
  //   - occasionally repack estore
  //

  class q_t
  { typedef struct e_t_ {int dist; uint32_t *tile; struct e_t_* next;}* e_t;
    e_t head,tail;

    // block allocate elements
    // - have to do this or lock's during free (called by ~q_t()) get too time consuming
    e_t estore;
    size_t estore_sz;
    size_t estore_i;
    e_t elem() { // alloc new element
      estore_i++;
      if(estore_i>=estore_sz)
        return 0;
      return estore+(estore_i-1);
    }
    void free_elem(e_t e) { /* release element -- no op */ }
    e_t bind(uint32_t* v,int dist) {
      e_t e=elem();
      if(!e) return 0;
      e->tile=v; e->next=NULL; e->dist=dist;
      return e;
    }
  public:
    q_t(size_t n):head(0),tail(0),estore(0),estore_sz(n),estore_i(0) {estore=(e_t)calloc(n,sizeof(struct e_t_));}
    ~q_t() { free(estore); } //e_t e=head; while(e) {e_t t=e->next; free_elem(e); e=t; } estore_i=0;}
    q_t* push(uint32_t* v, int dist) throw(const char*) {
      e_t e=0;
      if(!v) return this; // no-op -- to allow implicit filtering of inputs
      e=bind(v,dist);
      if(!e) throw "allocation error";    // memory allocation error
      if(!head) { head=tail=e; }          // first elem. must test head bc of how I pop
      else      { tail->next=e; tail=e;}
      return this;
    }
    uint32_t* pop(int *dist) {
      uint32_t* v=0;
      int d=-1;
      e_t e=head;
      if(head) { head=head->next; v=e->tile; d=e->dist; free_elem(e);}
      if(dist) *dist=d;
      return v;
    }
  };

  int StageTiling::minDistTo(
    uint32_t search_mask,uint32_t search_flags, // area to search 
    uint32_t query_mask,uint32_t query_flags)  // tile to find
  { 
    int dist=0;
    const unsigned w=attr_->dims[0],
                   h=attr_->dims[1];
    const int offsets[] = {-w-1,-w,-w+1,-1,1,w-1,w,w+1}; // moves
    uint32_t *c = AUINT32(attr_)+cursor_,
             *beg = AUINT32(attr_)+current_plane_offset_,
             *end = beg+sz_plane_nelem_;    
    q_t q(w*h);
    
    for(uint32_t *t=(uint32_t*)beg;t<end;++t) // Reset Reserved
      *t = t[0]&~Reserved;

    #define isvalid(p)    ((*(p)&(search_mask|Reserved)) == search_flags)
    #define isinbounds(p) (beg<=(p) && (p)<end)
    #define maybe(p)      ( (isvalid(p)&&isinbounds(p)) ?(p):NULL)
    try
    { q.push(maybe(c),0);
      while(c=q.pop(&dist))
      { *c |= Reserved; // mark it as looked at
        if((*c&query_mask)==query_flags) {goto Finalize;}
        for(int i=0;i<countof(offsets);++i)
          q.push(maybe(c+offsets[i]),dist+1);
      }
      dist=0; // - no tiles found or started in a bad spot
    }
    catch(const char* msg)
    { debug("%s(%d): %s()\r\n\t%s\r\n",__FILE__,__LINE__,__FUNCTION__,msg);
      dist = -1;
    }
    #undef isvalid
    #undef isinbounds
    #undef maybe
  Finalize:
    for(uint32_t *t=(uint32_t*)beg;t<end;++t) // Reset Reserved
      *t = t[0]&~Reserved;
    return dist;
  }

  void StageTiling::notifyDone(size_t index, const Vector3f& pos, uint32_t sts)
  { TListeners::iterator i;
    for(i=listeners_.begin();i!=listeners_.end();++i)
      (*i)->tile_done(index,pos,sts);
  }

  void StageTiling::notifyNext(size_t index, const Vector3f& pos)
  { TListeners::iterator i;
    for(i=listeners_.begin();i!=listeners_.end();++i)
      (*i)->tile_next(index,pos);
  }

  const Vector3f StageTiling::computeCursorPos()
  {
    mylib::Coordinate *c = mylib::Idx2CoordA(attr_,cursor_);
    mylib::Dimn_Type *d = (mylib::Dimn_Type*)ADIMN(c);
    Vector3z r;
    r << d[0],d[1],d[2];
    Vector3f pos = latticeToStage_ * r.transpose().cast<float>();
    Free_Array(c);
    return pos;
  }

  void StageTiling::getCursorLatticePosition(int *x,int *y,int *z)
  {
      mylib::Coordinate *c=mylib::Idx2CoordA(attr_,cursor_);
      mylib::Dimn_Type *d=(mylib::Dimn_Type*)ADIMN(c);
      if(x) *x=d[0];
      if(y) *y=d[1];
      if(z) *z=d[2];
      Free_Array(c);      
  }

  float StageTiling::plane_mm()
  {
    Vector3f r(0,0,(float)plane());
    r = latticeToStageTransform() * r * 0.001;
    return r(2);
  }

}} // end namespace fetch::device
