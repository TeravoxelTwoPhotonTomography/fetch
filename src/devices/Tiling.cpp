#include "Tiling.h"
#include "Stage.h"

#include <Eigen/Core>
using namespace Eigen;

namespace mylib
{
#include <flood.fill.h>
}

#include <iostream>
#define SHOW(e) std::cout << "---" << std::endl << #e << " is " << std::endl << (e) << std::endl << "~~~"  << std::endl;

namespace fetch {
namespace device {
  //////////////////////////////////////////////////////////////////////
  //  StageTiling  /////////////////////////////////////////////////////
  //////////////////////////////////////////////////////////////////////

  //  Constructors  ////////////////////////////////////////////////////
  StageTiling::StageTiling(const device::StageTravel &travel,     
                           const FieldOfViewGeometry &fov,        
                           const Mode                 alignment)
    :
      mask_(NULL),
      leftmostAddressable_(0),
      cursor_(0),
      current_plane_offest_(0),
      sz_plane_nelem_(0),
      latticeToStage_(),
      fov_(fov)
  { device::StageTravel t = travel;
    computeLatticeToStageTransform_(fov,alignment);
    initMask_(computeLatticeExtents_(travel));
    markAddressable_(&t);
  }

  //  Destructor  /////////////////////////////////////////////////////
  StageTiling::~StageTiling()
  { if(mask_)           Free_Array(mask_);
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
    SHOW(sabox);

    Matrix<float,3,8> labox; // vertices of the cube, lattice aligned
    labox.noalias() = latticeToStage_.inverse() * sabox.transpose();
    SHOW(labox);

    Vector3f maxs,mins;
    maxs.noalias() = labox.rowwise().maxCoeff();
    mins.noalias() = labox.rowwise().minCoeff();
    SHOW(mins);
    SHOW(maxs);

    latticeToStage_.translate(mins);
    Vector3z c((maxs-mins).cast<size_t>());
    SHOW(c);

    mylib::Coordinate* out = mylib::Coord3(c(2)+1,c(1)+1,c(0)+1); //shape of the lattice
    return out;
  }

  //  initMask_  ///////////////////////////////////////////////////////
  //

  void StageTiling::initMask_(mylib::Coordinate *shape)
  { mask_ = mylib::Make_Array_With_Shape(
      mylib::PLAIN_KIND,
      mylib::UINT8_TYPE,
      shape); // shape gets free'd here

    memset(mask_->data,0,sizeof(uint8_t)*mask_->size); //unset
    sz_plane_nelem_ = mask_->dims[0] * mask_->dims[1];
  }

  //  markAddressable_  ////////////////////////////////////////////////
  //

	struct FloodFillArgs
	{ StageTiling         *self;
	  device::StageTravel *travel;
	};
	  
  static mylib::boolean isInBox(mylib::Indx_Type p, void *argt)
  { 
  	FloodFillArgs *args         = (FloodFillArgs*)argt;
  	StageTiling   *self         = args->self;
  	device::StageTravel *travel = args->travel;
  	mylib::Coordinate *coord    = mylib::Idx2CoordA(args->self->mask(),p);
    Map<Vector3f> c((float*)(coord->data));
  	Vector3f r                  = self->latticeToStageTransform() * r;
    return ( (travel->x.min<r(0)) && (r(0)>travel->x.max) ) &&
           ( (travel->y.min<r(1)) && (r(1)>travel->y.max) ) &&
           ( (travel->z.min<r(2)) && (r(2)>travel->z.max) );
  }

  typedef uint8_t uint8;
  static void actionMarkAddressable(mylib::Indx_Type p, void *arga)
  { 
  	FloodFillArgs *args            = (FloodFillArgs*)arga;
  	AUINT8(args->self->mask())[p] |= StageTiling::Addressable;
  }

  void StageTiling::markAddressable_(device::StageTravel *travel)
  { 
    FloodFillArgs args = {this,travel};
#pragma warning(push)
#pragma warning(disable:4244) // conversion from 'double' to 'mylib::Dimn_Type' might lose data
    mylib::Coordinate *mid = mylib::Coord3( 
      (travel->z.max-travel->z.min)/2.0,
      (travel->y.max-travel->y.min)/2.0, 
      (travel->x.max-travel->x.min)/2.0);
#pragma warning(push)

    leftmostAddressable_ = mylib::Find_Leftmost_Seed(
      mask_,
      1,                              /* share */
      0,                              /* conn  */
      Coord2IdxA(mask_,mid),          /* seed  */
      &args,isInBox);
    Flood_Object(mask_,
      1,                              /* share */
      0,                              /* conn  */
      leftmostAddressable_,           /* seed  */
      &args,isInBox,
      NULL,NULL,
      &args,actionMarkAddressable);
  }

  //  resetCursor  /////////////////////////////////////////////////////
  //
  typedef mylib::Dimn_Type Dimn_Type;
  void StageTiling::resetCursor()
  { cursor_ = leftmostAddressable_;

    mylib::Coordinate *c = mylib::Idx2CoordA(mask_,cursor_);
    current_plane_offest_ = ADIMN(c)[2] * sz_plane_nelem_;
    mylib::Free_Array(c);
  }

  //  nextInPlanePosition  /////////////////////////////////////////////
  //                           
  #define ON_PLANE(e)    ((e) < (current_plane_offest_ + sz_plane_nelem_))
  #define ON_LATTICE(e)  ((e) < (mask_->size))
  bool StageTiling::nextInPlanePosition(Vector3f &pos)
  { uint8_t* mask = AUINT8(mask_);
    uint8_t attr = Addressable | Active;

    while( (mask[cursor_] & attr) != attr
        && ON_PLANE(cursor_) )
        ++cursor_;

    if(ON_PLANE(cursor_))
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
  { uint8_t* mask = AUINT8(mask_);
    uint8_t attrmask = Addressable | Active | Done,
            attr     = Addressable & Active;

    while( (mask[cursor_] & attrmask) != attr
        && ON_LATTICE(cursor_) )
        ++cursor_;

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
  { uint8_t *m = AUINT8(mask_) + cursor_;
    *m |= Done;
    if(success)
      *m |= Success;
    notifyDone(cursor_,computeCursorPos(),*m);
  }

  void StageTiling::notifyDone(size_t index, const Vector3f& pos, uint8_t sts)
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
    mylib::Coordinate *c = mylib::Idx2CoordA(mask_,cursor_);     
    Map<Vector3z> r((size_t*)ADIMN(c));
    Vector3f pos = latticeToStage_ * r.transpose().cast<float>();
    Free_Array(c);	
    return pos;
  }

}} // end namespace fetch::device
