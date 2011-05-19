#pragma once
#include <QtGui>
#include <devices/stage.h>
#include <ui/uiunits.h>

#include <Eigen/Core>
#include "devices/stage.h"
#include "devices/tiling.h"
#include "AgentController.h"

using namespace Eigen;

namespace fetch {
namespace ui {

  class TilingControllerListener:public QObject, public device::StageListener //forwards callbacks through qt signals
  {      
    Q_OBJECT
  public:
    void tile_done( size_t index, const Vector3f& pos,uint8_t sts )        {emit sig_tile_done(index,sts);}
    void tiling_changed( device::StageTiling *tiling )                     {emit sig_tiling_changed(tiling);}
    void tile_next( size_t index, const Vector3f& pos )                    {emit sig_tile_next(index);}

  signals:
    void sig_tile_done( size_t index, uint8_t sts );
    void sig_tiling_changed( device::StageTiling *tiling );
    void sig_tile_next( size_t index );
  };

  class TilingController:public QObject
  {
    Q_OBJECT

  public:
    typedef Matrix<float,3,4>         TRectVerts;
    typedef Transform<float,3,Affine> TTransform;

    TilingController(device::StageTiling *tiling, QObject* parent=0);

    void fovGeometry      (TRectVerts *out);
    void latticeTransform (TTransform *out);
    void latticeShape     (unsigned *width, unsigned *height);

    void markActive(const QPainterPath& path);

  public slots:
    void updateTiling(device::StageTiling *tiling)                         {tiling_=tiling; emit changed();}
    void stageAttached()                                                   {emit show(true);}
    void stageDetached()                                                   {emit show(false);}

  signals:
    void show(bool tf);
    void changed();
    void tileDone(size_t itile);
    void nextTileRequest(size_t itile);                                    // really should be nextTileRequested
    // other ideas: imaging started, move start, move end

  private:
    device::StageTiling *tiling_;
    TilingControllerListener listener_;

  };

  class PlanarStageController:public QObject
  { Q_OBJECT
    public:

      static const units::Length Unit = units::MM;

      PlanarStageController(device::Stage *stage, QObject *parent=0) : QObject(parent), stage_(stage), agent_controller_(stage->_agent) {}

      QRectF  travel()                                                     { device::StageTravel t; stage_->getTravel(&t); return QRectF(QPointF(t.x.min,t.y.min),QPointF(t.x.max,t.y.max)); }
      QPointF velocity()                                                   { float vx,vy,vz; stage_->getVelocity(&vx,&vy,&vz); return QPointF(vx,vy); }
      QPointF pos()                                                        { float  x, y, z; stage_->getPos(&x,&y,&z); return QPointF(x,y); } 

    signals:

      void moved(QPointF pos);

    public slots:
      
      void setVelocity(QPointF v)                                          { stage_->setVelocity(v.x(),v.y(),0.0); }
      void moveTo(QPointF r)                                               { float  x, y, z; stage_->getPos(&x,&y,&z); stage_->setPos(r.x(),r.y(),z);       emit moved(r);}
      void moveRel(QPointF dr)                                             { float  x, y, z; stage_->getPos(&x,&y,&z); stage_->setPos(x+dr.x(),y+dr.y(),z); emit moved( QPointF(x+dr.x(),y+dr.y()));} 

    private:

      device::Stage   *stage_;
      AgentController  agent_controller_;

  };


}} //end fetch::ui
