#pragma once
#include "stage.pb.h"
namespace mylib {
#include <array.h>
}    
#include <Eigen/Core>
#include <Eigen/Geometry>

using namespace Eigen;

namespace fetch {
  namespace device { struct StageTravel; }

  typedef Matrix<size_t,1,3> Vector3z;

  //////////////////////////////////////////////////////////////////////
  //  FieldOfViewGeometry  /////////////////////////////////////////////
  //////////////////////////////////////////////////////////////////////

  struct FieldOfViewGeometry
  { Vector3f field_size_um_;
    float    rotation_radians_;

    FieldOfViewGeometry(float x, float y, float z, float radians) :
          field_size_um_(x,y,z), rotation_radians_(radians) {}
    FieldOfViewGeometry(const cfg::device::FieldOfViewGeometry &cfg) {update(cfg);}

    void update(const cfg::device::FieldOfViewGeometry &cfg)
         { field_size_um_ << cfg.x_size_um(),cfg.y_size_um(),cfg.z_size_um();
           rotation_radians_ = cfg.rotation_radians();
         }
      
  };

  //////////////////////////////////////////////////////////////////////
  //  StageTiling  /////////////////////////////////////////////////////
  //////////////////////////////////////////////////////////////////////
  class StageTiling
  {
    typedef Transform<float,3,Affine> TTransform;

    mylib::Array              *mask_;
    mylib::Indx_Type           leftmostAddressable_;
    mylib::Indx_Type           cursor_;
    mylib::Indx_Type           current_plane_offest_;
    mylib::Indx_Type           sz_plane_nelem_;
    TTransform                 latticeToStage_;

  public:
    typedef fetch::cfg::device::Stage_TilingMode Mode;    

    enum Flags
    { 
      Addressable = 1,
      Active      = 2,
      Done        = 4,
      Success     = 8
    };

             StageTiling(const device::StageTravel& travel,
                         const FieldOfViewGeometry& fov,
                         const Mode                 alignment);
    virtual ~StageTiling();

    void     resetCursor();
    bool     nextInPlanePosition(Vector3f& pos);
    bool     nextPosition(Vector3f& pos);
    void     markDone(bool success);
    
    inline mylib::Array* mask() {return mask_;}
    inline const TTransform& latticeToStageTransform() {return latticeToStage_; }

  protected:
    void computeLatticeToStageTransform_
                        (const FieldOfViewGeometry& fov,
                         const Mode                 alignment);
    mylib::Coordinate* computeLatticeExtents_(const device::StageTravel& travel);
    void initMask_(mylib::Coordinate *shape);

    void markAddressable_(device::StageTravel *travel);

  };

} //end namespace fetch

