#include "Tiling.h"

namespace fetch {

  //////////////////////////////////////////////////////////////////////
  //  StageTiling  /////////////////////////////////////////////////////
  //////////////////////////////////////////////////////////////////////

  //  Constructors  ////////////////////////////////////////////////////
  StageTiling::StageTiling(const StageTravel         &travel,     
                           const FieldOfViewGeometry &fov,        
                           const Mode                 alignment)
    :
      mask_(NULL),
      latticeToStage_()
  { StageTravel t = travel;
    computeLatticeToStageTransform_(fov,alignment);
    initMask_(computeLatticeExtents_(travel));
    markAddressable_(&t);
  }

  //  Destructors  /////////////////////////////////////////////////////
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
    Vector3f sc = Map<Vector3f>(fov.field_size_um_);
    switch(alignment)
    { case PixelAligned:
        // Rotate the lattice
        latticeToStage_
          .rotate(fov.rotation_radians)
          .scale(sc);
        break;
      case StageTravel:
        // Shear the lattice
        float th = fov.rotation_radians;
        latticeToStage_.linear() = 
          (Matrix3f() << 
            1.0f/cos(th), -sin(th), 0,
                       0,  cos(th), 0,
                       0,        0, 1).finished()
          .scale(sc);
        break;
    }
  }
  
  //  computeLatticeExtents_  //////////////////////////////////////////
  //
  //  Find the range of indexes that cover the stage.
  //  This is done by applying the inverse latticeToStage_ transform to the
  //    rectangle described by the stage extents, and then finding extreema.
  //  The latticeToStage_ transfrom is adjusted so the minimal extrema is the
  //    origin.  That is, [min extremal] = T(0,0,0).
  mylib::Coordinate* StageTiling::computeLatticeExtents_(const StageTravel  &travel)
  {
    Matrix<float,8,3> sabox; // vertices of the cube, stage aligned
    sabox 
      << travel.x.min << travel.y.min << travel.z.min
      << travel.x.min << travel.y.max << travel.z.min
      << travel.x.max << travel.y.max << travel.z.min
      << travel.x.max << travel.y.min << travel.z.min

      << travel.x.min << travel.y.min << travel.z.max
      << travel.x.min << travel.y.max << travel.z.max
      << travel.x.max << travel.y.max << travel.z.max
      << travel.x.max << travel.y.min << travel.z.max;

    Matrix<float,3,8> labox; // vertices of the cube, lattice aligned
    labox = latticeToStage_.inverse() * sabox.transpose();

    Vector<float,3> maxs,mins;
    maxs = labox.rowwise().maxCoeff();
    mins = labox.rowwise().minCoeff();

    latticeToStage_.translate(-mins);
    Vector<mylib::Dimn_Type,3> c((max-min).cast<mylib::Dimn_Type>());
    return mylib::Coord3(c(0)+1,c(1)+1,c(2)+1); //shape of the lattice
  }

  //  initMask_  ///////////////////////////////////////////////////////
  //

  void StageTiling::initMask_(mylib::Coordinate *shape)
  { mask_ = mylib::Make_Array_With_Shape(
      mylib::PLAIN_KIND,
      mylib::UINT8_TYPE,
      shape);

    memset(mask_,0,sizeof(uint8_t)*mask_->size); //unset

    //TODO: set stage addressable lattice points

  }


	struct FloodFillArgs
	{ StageTiling *self;
	  StageTravel *travel;
	};
	  
  static mylib::boolean isInBox(Indx_Type p, void *argt)
  { 
  	FloodFillArgs *args      = (FloodFillArgs)argt;
  	StageTiling   *self    = args->self;
  	StageTravel *travel      = args->travel;
  	mylib::Coordinate *coord = mylib::Idx2CoordA(args->self->mask_,p);
  	Vector3f c               = Map<Vector3f>(coord->data),
             r               = arg->self->latticeToStage_ * r;
    return ( (travel.x.min<r(0)) && (r(0)>travel.x.max) ) &&
           ( (travel.y.min<r(1)) && (r(1)>travel.y.max) ) &&
           ( (travel.z.min<r(2)) && (r(2)>travel.z.max) );
  }

  static void totalDoNothing(Size_Type, void *)
  { }

  static void actionMarkAddressable(Indx_Type p, void *arga)
  { 
  	FloodFillArgs *args         = (FloodFillArgs)arga;
  	args->self->mask_->data[p]| = StageTiling::Addressable;
  }

  void StageTiling::markAddressable_(StageTiling *travel)
  { 
    FloodFillArgs args(this,travel);
    Indx_Type p = Find_Leftmost_Seed(mask_,1,0/*conn*/,0/*seed*/,&args,isInBox);
    Flood_Object(mask_,1,0,p,
        &args,isInBox,
        NULL,totalDoNothing,
        &args,actionMarkAddressable);
  }

} // end namespace fetch
