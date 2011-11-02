#pragma once
#include <QtGui>
#include <devices/stage.h>
#include <ui/uiunits.h>

#include <Eigen/Core>
#include "devices/stage.h"
#include "devices/tiling.h"
#include "ui/AgentController.h"

using namespace Eigen;

namespace fetch {
namespace ui {

  class TilingControllerListener:public QObject, public device::StageListener //forwards callbacks through qt signals
  {      
    Q_OBJECT
  public:
    void tile_done( size_t index, const Vector3f& pos,uint32_t sts )       {emit sig_tile_done(index,(unsigned int)sts);}
    void tiling_changed( device::StageTiling *tiling )                     {emit sig_tiling_changed(tiling);}
    void tile_next( size_t index, const Vector3f& pos )                    {emit sig_tile_next((unsigned int)index);}
    void fov_changed(const device::FieldOfViewGeometry *fov)               {emit sig_fov_changed(fov->field_size_um_[0],fov->field_size_um_[1],fov->rotation_radians_);}      
    void moved(void)                                                       {emit sig_moved();}  
    
  signals:
    void sig_tile_done( unsigned index, unsigned int sts );
    void sig_tiling_changed( device::StageTiling *tiling );
    void sig_tile_next( unsigned index );
    void sig_fov_changed(float w_um, float h_um, float rotation_radians);
    void sig_moved();
  };

  class TilingController:public QObject
  {
    Q_OBJECT

  public:
    typedef Matrix<float,3,4>         TRectVerts;
    typedef Transform<float,3,Affine> TTransform;

    TilingController(device::Stage *stage, device::StageTiling *tiling=NULL, QObject* parent=0);

    inline TilingControllerListener* listener()                            {return &listener_;}

    inline bool is_valid()                                                 {return tiling_!=NULL;}

    bool fovGeometry      (TRectVerts *out);                               // returns false if tiling is invalid
    bool latticeTransform (TTransform *out);                               // returns false if tiling is invalid
    bool latticeTransform (QTransform *out);                               // returns false if tiling is invalid - QtVersion
    bool latticeShape     (unsigned *width, unsigned *height);             // returns false if tiling is invalid
    bool latticeShape     (QRectF *out);                                   // returns false if tiling is invalid
    bool latticeAttrImage (QImage *out);                                   // returns false if tiling is invalid
    bool stageAlignedBBox (QRectF *out);                                   // returns false if tiling is invalid

    bool markAddressable();                                                // returns false if tiling is invalid
    bool markActive(const QPainterPath& path);                             // returns false if tiling is invalid
    bool markInactive(const QPainterPath& path);                           // returns false if tiling is invalid

    bool mapToIndex(const Vector3f & stage_coord, unsigned *index);        // returns false if tiling is invalid or if stage_coord is oob

    QAction* saveDialogAction() {return load_action_;}
    QAction* loadDialogAction() {return save_action_;}

  public slots:
    void update(device::StageTiling *tiling)                               {tiling_=tiling; markAddressable(); emit changed(); emit show(tiling_!=NULL);}
    void stageAttached()                                                   {emit show(true);}
    void stageDetached()                                                   {emit show(false);}    
    void updatePlane()                                                     {markAddressable();}

    void saveViaFileDialog();
    void loadViaFileDialog();
    void saveToFile(const QString& filename);
    void loadFromFile(const QString& filename);

  signals:
    void show(bool tf);
    void changed();
    void planeChanged();
    void tileDone(unsigned itile,unsigned int attr);
    void nextTileRequest(unsigned itile);                                  // really should be nextTileRequested
    void fovGeometryChanged(float w_um, float h_um, float rotation_radians);
    // other ideas: imaging started, move start, move end

  private:

    device::Stage       *stage_;
    device::StageTiling *tiling_;
    TilingControllerListener listener_;
    QAction             *load_action_,
                        *save_action_;

    bool mark(     const QPainterPath& path,                               // path should be in scene coords (um)
                   device::StageTiling::Flags attr, 
                   QPainter::CompositionMode mode );
    bool mark_all( device::StageTiling::Flags attr,                        // marks the whole plane with the attribute
                   QPainter::CompositionMode mode );
  };
  

  class PlanarStageControllerListener:public QObject, public device::StageListener
  {
    Q_OBJECT
  public:
    virtual void moved(void) {emit sig_moved();}
    virtual void velocityChanged(void) {emit sig_velocityChanged();}
  signals:
    void sig_moved();
    void sig_velocityChanged();

  };

  class PlanarStageController:public QObject
  { 
    Q_OBJECT    
    public:

      static const units::Length Unit = units::MM;

      PlanarStageController(device::Stage *stage, QObject *parent=0);// : QObject(parent), stage_(stage), agent_controller_(stage->_agent) {}

      QRectF  travel()                                                     { device::StageTravel t; stage_->getTravel(&t); return QRectF(QPointF(t.x.min,t.y.min),QPointF(t.x.max,t.y.max)); }
      QPointF velocity()                                                   { float vx,vy,vz; stage_->getVelocity(&vx,&vy,&vz); return QPointF(vx,vy); }
      QPointF pos()                                                        { float  x, y, z; stage_->getPos(&x,&y,&z); return QPointF(x,y); } 
      QPointF target()                                                     { float  x, y, z; stage_->getTarget(&x,&y,&z); return QPointF(x,y); } 

      TilingController* tiling()                                           {return &tiling_controller_;}
      PlanarStageControllerListener* listener()                            {return &listener_;}

      device::Stage *stage()                                               {return stage_;}
                                                                     
  signals:
      void moved();            // eventually updates the imitem's position      
      void moved(QPointF pos); // eventually updates the imitem's position
      void velocityChanged();

    public slots:
      
      void setVelocity(QPointF v)                                          { stage_->setVelocity(v.x(),v.y(),0.0); }
      void moveTo(QPointF r)                                               { float  x, y, z; stage_->getPos(&x,&y,&z); stage_->setPosNoWait(r.x(),r.y(),z);       emit moved(r);}
      void moveRel(QPointF dr)                                             { float  x, y, z; stage_->getPos(&x,&y,&z); stage_->setPosNoWait(x+dr.x(),y+dr.y(),z); emit moved( QPointF(x+dr.x(),y+dr.y()));} 

      void updateTiling()                                                  { tiling_controller_.update(stage_->tiling());}
      void invalidateTiling()                                              { tiling_controller_.update(NULL);}

    private:

      device::Stage   *stage_;
      AgentController  agent_controller_;
      TilingController tiling_controller_; 

      PlanarStageControllerListener listener_;
  };


}} //end fetch::ui
