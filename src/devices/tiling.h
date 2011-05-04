#pragma once

namespace mylib {
#include <array.h>
}
    
#include <Eigen/Core>
#include <Eigen/Geometry>

namespace fetch {

  typedef Matrix<size_t,1,3> Vector3z

  //////////////////////////////////////////////////////////////////////
  //  FieldOfViewGeometry  /////////////////////////////////////////////
  //////////////////////////////////////////////////////////////////////

  struct FieldOfViewGeometry
  { Vector3f field_size_um_;
    float    rotation_radians_;

    FieldOfViewGeometry(float x, float y, float z, float radians) :
      field_size_um_(x,y,z), rotation_radians(radians_) {}
  };

  //////////////////////////////////////////////////////////////////////
  //  StageTiling  /////////////////////////////////////////////////////
  //////////////////////////////////////////////////////////////////////
  class StageTiling
  {
    typedef Transform<float,3,Affine> TTransform;

    mylib::Array              *mask_;
    TTransform                 latticeToStage_;

    enum Mode
    { PixelAligned = 0,
      StageAligned
    };

    enum Flags
    { 
      Active  = 1,
      Success = 2,
    };

  public:
             StageTiling(const StageTravel         &travel,
                         const FieldOfViewGeometry &fov,
                         const Mode                 alignment);
    virtual ~StageTiling();

  protected:
    void computeLatticeToStageTransform_
                        (const FieldOfViewGeometry &fov,
                         const Mode                 alignment);
    mylib::Coordinate* computeLatticeExtents_(const StageTravel  &travel);
    void initMask_(Coordinate *shape);

  };

} //end namespace fetch

