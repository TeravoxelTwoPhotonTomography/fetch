#include "ui/StageView.h"
#include "ui/uiunits.h"

namespace fetch {
namespace ui {

using namespace units;



/*
 *    STAGE VIEW
 */
StageView::StageView(PlanarStageController *stageControl, QGraphicsItem *parent)
        : QGraphicsObject(parent)
        , bbox_meters_(0,0,0.1,0.1)
        , pos_meters_(0.0,0.0)
        , pen_(Qt::white)
        , target_pen_(Qt::yellow)
        , brush_(Qt::black)
        , control_(stageControl)
{ bbox_meters_ = cvt<M,PlanarStageController::Unit>(control_->travel());
  pos_meters_  = cvt<M,PlanarStageController::Unit>(control_->pos());
  //bbox_meters_.moveCenter(pos_meters_);

  // draw 1-pixel wide no matter what the view transform is
  pen_.setWidth(0);
  target_pen_.setWidth(0);


  stage_poll_timer_.setInterval(100/*ms*/);
  connect(&stage_poll_timer_,SIGNAL(timeout()),this,SLOT(poll_stage()));
  
  connect(control_,SIGNAL(moved()),&stage_poll_timer_,SLOT(start()));     // Start polling when the stage starts moving
}

StageView::~StageView()
{ stage_poll_timer_.stop();
}

void StageView::poll_stage()
{ update();    
}

QRectF
StageView::boundingRect() const
{ return cvt<PIXEL_SCALE,M>(bbox_meters_);}

void
StageView::paint(QPainter *p, const QStyleOptionGraphicsItem *o, QWidget *widget)
{   

  p->setPen(pen_);
  p->setBrush(brush_);
  QRectF bbox = cvt<PIXEL_SCALE,M>(bbox_meters_);
  p->drawRect(bbox);

  QPointF r = cvt<PIXEL_SCALE,PlanarStageController::Unit>(control_->pos());
  p->drawLine(QPointF(bbox.left()+10.0,r.y()),
              QPointF(bbox.right()-10.0,r.y()));
  p->drawLine(QPointF(r.x(),bbox.top()+10.0 ),
              QPointF(r.x(),bbox.bottom()-10.0));
              
  p->setPen(target_pen_);
  r = cvt<PIXEL_SCALE,PlanarStageController::Unit>(control_->target());
  p->drawLine(QPointF(bbox.left(),r.y()),
              QPointF(bbox.right(),r.y()));
  p->drawLine(QPointF(r.x(),bbox.top() ),
              QPointF(r.x(),bbox.bottom()));
  //qDebug()<<"10 cm to " << cvt<MM,CM>(10.0) << " mm";
  //qDebug()<<"StageView - paint - " << *o;  
  //qDebug()<<"StageView - paint - lod()  - " << o->levelOfDetailFromTransform(p->worldTransform());
  //qDebug()<<"StageView - paint - exrect - " << o->exposedRect;
  //qDebug()<<"StageView - paint -   rect - " << o->rect;
  //if(widget)
  //  qDebug()<<"StageView - paint - " << widget->geometry();
  //else
  //  qDebug()<<"StageView - paint - NONE";

}

void StageView::mousePressEvent( QGraphicsSceneMouseEvent *event )
{
  if(event->modifiers()==Qt::ControlModifier)
  {
    control_->moveTo(cvt<PlanarStageController::Unit,PIXEL_SCALE>(event->scenePos()));
    event->accept();
  }
  event->ignore();
}

}} //end namespace fetch::ui
