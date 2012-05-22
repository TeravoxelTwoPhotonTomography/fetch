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

#define DEBUG__WRITE_IMAGES
#define DEBUG__SHOW

#ifdef DEBUG__SHOW
#define SHOW(e) std::cout << "---" << std::endl << #e << " is " << std::endl << (e) << std::endl << "~~~"  << std::endl;
#else
#define SHOW(e)
#endif



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
      leftmostAddressable_(0),
      cursor_(0),
      current_plane_offset_(0),
      sz_plane_nelem_(0),
      latticeToStage_(),
      fov_(fov),
      travel_(travel)
  { 
    computeLatticeToStageTransform_(fov,alignment);
    initAttr_(computeLatticeExtents_(travel_));
    //markAddressable_(&travel_);
  }

  //  Destructor  /////////////////////////////////////////////////////
  StageTiling::~StageTiling()
  { if(attr_)           Free_Array(attr_);
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
    case Mode::Stage_TilingMode_PixelAligned:
        // Rotate the lattice
        latticeToStage_
          .rotate( AngleAxis<float>(fov.rotation_radians_,Vector3f::UnitZ()) )
          .scale(sc);          
        //SHOW(latticeToStage_.matrix());
        return;
    case Mode::Stage_TilingMode_StageAligned:
        // Shear the lattice
        float th = fov.rotation_radians_;
        latticeToStage_.linear() = 
          (Matrix3f() << 
            1.0f/cos(th), -sin(th), 0,
                       0,  cos(th), 0,
                       0,        0, 1).finished();
        latticeToStage_.scale(sc);
        return;
    }
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
  { attr_ = mylib::Make_Array_With_Shape(
      mylib::PLAIN_KIND,
      mylib::UINT32_TYPE,
      shape); // shape gets free'd here

    memset(attr_->data,0,sizeof(uint32_t)*attr_->size); //unset
    sz_plane_nelem_ = attr_->dims[0] * attr_->dims[1];
  }

  //  resetCursor  /////////////////////////////////////////////////////
  //  
  
#define ON_PLANE(e)    ((e) < (current_plane_offset_ + sz_plane_nelem_))
#define ON_LATTICE(e)  ((e) < (attr_->size))

  typedef mylib::Dimn_Type Dimn_Type;
  void StageTiling::resetCursor()
  { cursor_ = 0;

   uint32_t* mask    = AUINT32(attr_);
   uint32_t attrmask = Addressable | Active | Done,
            attr     = Addressable | Active;
    
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

  //  nextInPlanePosition  /////////////////////////////////////////////
  //                           

  bool StageTiling::nextInPlanePosition(Vector3f &pos)
  { uint32_t* mask = AUINT32(attr_);
    uint32_t attrmask = Addressable | Active | Done,
             attr     = Addressable | Active;

    do{++cursor_;}
    while( (mask[cursor_] & attrmask) != attr
        && ON_PLANE(cursor_) );

    if(ON_PLANE(cursor_) &&  (mask[cursor_] & attrmask) == attr)
    { pos = computeCursorPos();
      notifyNext(cursor_,pos);
      return true;
    } else
    { return false;
    }
  }

  //  nextInPlaneExplorablePosition  /////////////////////////////////////////////
  //                           

  /** Return the next tile-position to explore in the current tile-plane.
      
      Exclude tiles that are not Addressable, or that have already been 
      marked Active or Done.
  */

  bool StageTiling::nextInPlaneExplorablePosition(Vector3f &pos)
  { uint32_t* mask = AUINT32(attr_);
    uint32_t attrmask = Explorable | Addressable | Active | Done,
             attr     = Explorable | Addressable;

    do{++cursor_;}
    while( (mask[cursor_] & attrmask) != attr
        && ON_PLANE(cursor_) );

    if(ON_PLANE(cursor_) &&  (mask[cursor_] & attrmask) == attr)
    { pos = computeCursorPos();
      notifyNext(cursor_,pos);
      return true;
    } else
    { return false;
    }
  }

  //  nextPosition  ////////////////////////////////////////////////////
  //
  bool StageTiling::nextPosition(Vector3f &pos)
  { uint32_t* mask    = AUINT32(attr_);
    uint32_t attrmask = Addressable | Active | Done,
             attr     = Addressable | Active;

    do {++cursor_;}
    while( (mask[cursor_] & attrmask) != attr
        && ON_LATTICE(cursor_) );

    if(ON_LATTICE(cursor_))
    { pos = computeCursorPos();
      notifyNext(cursor_,pos);
      return true;
    } else
    { return false;
    }
  }

  //  markDone  ////////////////////////////////////////////////////////
  //
  void StageTiling::markDone(bool success)
  { uint32_t *m = AUINT32(attr_) + cursor_;
    *m |= Done;
    if(!success)
      *m |= TileError;
    notifyDone(cursor_,computeCursorPos(),*m);
  } 

  //  markActive  ////////////////////////////////////////////////////////
  //
  void StageTiling::markActive()
  { uint32_t *m = AUINT32(attr_) + cursor_;
    *m |= Active;
    notifyDone(cursor_,computeCursorPos(),*m);
  } 

  //  fillHolesInActive  /////////////////////////////////////////////////
  //
  
  /// Private stack implementation for non-recursive flood fill.
  typedef struct _stack_t
  { size_t     n,cap;
    uint32_t **data;
  } stack_t;
  void grow(stack_t *s)
  { if(s->n==s->cap-1)
    { size_t newsize = s->cap*1.2+50;
      Guarded_Realloc((void**)&s->data,newsize,"grow stack");
      memset(s->data+s->cap,0,(newsize-s->cap)*sizeof(uint32_t*));
      s->cap=newsize;
    }
  }  
  void push(stack_t *s, uint32_t* v)
  { grow(s);
    s->data[s->n++] = v;
  }  
  uint32_t* pop(stack_t *s)
  { return s->data[--s->n];
  }    
  stack_t make_stack(size_t reserve)
  { stack_t s;
    s.n=0;
    s.data=(uint32_t**)Guarded_Calloc(s.cap=reserve,sizeof(uint32_t*),"make stack");    
    return s;    
  }
  void destroy_stack(stack_t *s)
  { if(s && s->data) free(s->data);
  }
    
  
  /** Fill holes in regions marked as active 
  
    Filled regions are 4-connected.
  */
  void StageTiling::fillHolesInActive()
  { resetCursor();
    uint32_t *c,
             *beg = AUINT32(attr_)+current_plane_offset_,
             *end = beg+sz_plane_nelem_;
    int is_open=0;
    const unsigned w=attr_->dims[0],
                   h=attr_->dims[1];    
    stack_t stack = make_stack(sz_plane_nelem_);
  
    for(c=beg;c<end;)
    { uint32 *n,*next;
      uint32 mask = Active | Reserved;
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
                (n[0]&Active)?"Active":".",
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
                        
      if(!is_open)     // second fill to mark interior as Active
      { mask = Active; // edges and self are labeled Active
        push(&stack,0);
        push(&stack,c);
        while(n=pop(&stack))
        {   unsigned x = (n-beg)%w,
                     y = (n-beg)/w;
#if 0
            debug("%3d %3d %15s %15s %15s %15s\n",x,y,
                (n[0]&Reserved)?"Reserved":".",
                (n[0]&Active)?"Active":".",
                (n[0]&Explorable)?"Explorable":".",
                (n[0]&Addressable)?"Addressable":".");
#endif
            *n |= Active; 
            if(!(y>=(h-1) || *(next=(n+w))&mask )) {*next|=Active; push(&stack,next);}  // down
            if(!(y<=0     || *(next=(n-w))&mask )) {*next|=Active; push(&stack,next);}  // up
            if(!(x<=0     || *(next=(n-1))&mask )) {*next|=Active; push(&stack,next);}  // left
            if(!(x>=(w-1) || *(next=(n+1))&mask )) {*next|=Active; push(&stack,next);}  // right
        } // end second fill
      }
           
      while(c++<end && *c&(Reserved|Active));      // move to next unreserved
    } // done searching for regions
    destroy_stack(&stack);
    for(c=beg;c<end;++c) // mark all unreserved
      *c = c[0]&~Reserved;
  }

  //  dilateActive  //////////////////////////////////////////////////////
  //                      
  #define countof(e) (sizeof(e)/sizeof(*e))
  /** Mark tiles as Active if they are 8-connected to an Active tile. */
  void StageTiling::dilateActive()
  { resetCursor();
    uint32_t *c,
             *beg = AUINT32(attr_)+current_plane_offset_,
             *end = beg+sz_plane_nelem_;
    const unsigned w=attr_->dims[0],
                   h=attr_->dims[1];
    unsigned       x,y,j;
    
    const unsigned offsets[] = {-w-1,-w,-w+1,-1,1,w-1,w,w+1};
    const unsigned top=1,left=2,bot=4,right=8; // bit flags
    const unsigned masks[]   = {top|left,top,top|right,left,right,bot|left,bot,bot|right};      
    const unsigned mark = Reserved|Active;
    for(c=beg;c<end;++c)          // mark original active tiles as reserved
      *c |= ((*c&Active)==Active)*Reserved;
    for(y=0;y<h;++y)
    { unsigned rowmask = ((y==0)*top)|((y==h-1)*bot);
      for(x=0;x<w;++x)
      { const unsigned colmask = ((x==0)*left)|((x==w-1)*right),
                          mask = rowmask|colmask;
        c=beg+y*w+x;
        for(j=0;j<countof(offsets);++j)
          if( (mask&masks[j])==0 && (c[offsets[j]]&mark)==mark)
          { *c|=Active; break; 
          }
      }
    }
    for(c=beg;c<end;++c)          // mark all unreserved
      *c = c[0]&~Reserved;
  }

  void StageTiling::notifyDone(size_t index, const Vector3f& pos, uint32_t sts)
  { 
    TListeners::iterator i;
    for(i=listeners_.begin();i!=listeners_.end();++i)
      (*i)->tile_done(index,pos,sts);
  }

  void StageTiling::notifyNext(size_t index, const Vector3f& pos)
  {
    TListeners::iterator i;
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

  size_t StageTiling::plane_mm()
  {
    Vector3f r(0,0,(float)plane());
    r = latticeToStageTransform() * r * 0.001;
    return (size_t)r(2);
  }

}} // end namespace fetch::device
