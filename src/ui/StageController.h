#pragma once
#include <QtGui>
#include <devices/stage.h>
#include <ui/uiunits.h>

namespace fetch {
namespace ui {

  class PlanarStageController:public QObject
  { Q_OBJECT
    public:

      static const units::Length Unit = units::MM;

      PlanarStageController(device::IStage *stage, QObject *parent=0) : QObject(parent), stage_(stage) {}

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

      device::IStage *stage_;

  };


}} //end fetch::ui
