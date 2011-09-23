#pragma once
#include <Eigen/Core>
#include "microscope.pb.h"

using namespace Eigen;


namespace fetch {
namespace device {

//////////////////////////////////////////////////////////////////////
//  FieldOfViewGeometry  /////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////

struct FieldOfViewGeometry
{ 
  Vector3f field_size_um_;
  Vector3f overlap_um_;
  float    rotation_radians_;

  FieldOfViewGeometry() : field_size_um_(0,0,0),  overlap_um_(0,0,0), rotation_radians_(0) {}
  FieldOfViewGeometry(float x, float y, float z, float radians) :
        field_size_um_(x,y,z), rotation_radians_(radians) {}
  FieldOfViewGeometry(const cfg::device::FieldOfViewGeometry &cfg) {update(cfg);}

  void update(const cfg::device::FieldOfViewGeometry &cfg)
       { 
         field_size_um_ << cfg.x_size_um(),cfg.y_size_um(),cfg.z_size_um();
         overlap_um_ << cfg.x_overlap_um(),cfg.y_overlap_um(),cfg.z_overlap_um();
         rotation_radians_ = cfg.rotation_radians();           
       }

  void write(cfg::device::FieldOfViewGeometry *cfg) const
  { cfg->set_x_overlap_um     (   overlap_um_[0]);
    cfg->set_y_overlap_um     (   overlap_um_[1]);
    cfg->set_z_overlap_um     (   overlap_um_[2]);
    cfg->set_x_size_um        (field_size_um_[0]);
    cfg->set_y_size_um        (field_size_um_[1]);
    cfg->set_z_size_um        (field_size_um_[2]);
    cfg->set_rotation_radians (rotation_radians_);
  }
    
};

bool operator==(const FieldOfViewGeometry &lhs,const FieldOfViewGeometry &rhs);

}} // end namespace fetch::device