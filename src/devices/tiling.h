#pragma once
#include "stage.pb.h"
namespace mylib {
#include <array.h>
}    
#include <Eigen/Core>
#include <Eigen/Geometry>
#include <Set>
#include <stdint.h>

using namespace Eigen;

namespace fetch {
namespace device { 

  struct StageTravel;

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
  struct StageListener;
  class StageTiling
  {
    typedef Transform<float,3,Affine> TTransform;
    typedef std::set<StageListener*>  TListeners;

    mylib::Array              *mask_;                                      // tile attribute database
    mylib::Indx_Type           leftmostAddressable_;                       // marks the first tile
    mylib::Indx_Type           cursor_;                                    // marks the current tile
    mylib::Indx_Type           current_plane_offest_;                      // marks the current plane
    mylib::Indx_Type           sz_plane_nelem_;                            // the size of a plane in the tile database
    TTransform                 latticeToStage_;                            // Transforms lattice coordinates to the tiles anchor point on the stage
    TListeners                 listeners_;                                 // set of objects to be notified of tiling events
    FieldOfViewGeometry        fov_;                                       // the geometry used to generate the tiling

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
    
    inline mylib::Array*     mask() {return mask_;}
    inline const TTransform& latticeToStageTransform()                     {return latticeToStage_; }
    inline const FieldOfViewGeometry& fov()                                {return fov_;}
    inline size_t plane()                                                  {return current_plane_offest_/sz_plane_nelem_; }

    inline void addListener(StageListener *listener)                       {listeners_.insert(listener);}
    inline void delListener(StageListener *listener)                       {listeners_.erase(listener);}

  protected:
    void computeLatticeToStageTransform_
                        (const FieldOfViewGeometry& fov,
                         const Mode                 alignment);
    mylib::Coordinate* computeLatticeExtents_(const device::StageTravel& travel);
    void initMask_(mylib::Coordinate *shape);

    void markAddressable_(device::StageTravel *travel);
    
    void notifyDone(size_t i, const Vector3f& pos, uint8_t sts);    
    void notifyNext(size_t i, const Vector3f& pos);  

    const Vector3f computeCursorPos();
  public:
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW
  };
  
}} //end namespace fetch