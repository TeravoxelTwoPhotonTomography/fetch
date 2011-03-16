#include "ui/StageView.h"
#include "ui/uiunits.h"

namespace fetch {
namespace ui {

using namespace units;

StageView::StageView(PlanarStageController *stageControl, QGraphicsItem *parent)
        : QGraphicsItem(parent)
        , bbox_meters_(0,0,0.1,0.1)
        , pos_meters_(0.0,0.0)
        , pen_(Qt::white)
        , brush_(Qt::black)
        , control_(stageControl)
{ bbox_meters_ = cvt<M,PlanarStageController::Unit>(control_->travel());
  pos_meters_  = cvt<M,PlanarStageController::Unit>(control_->pos());
  //bbox_meters_.moveCenter(pos_meters_);
}

StageView::~StageView()
{
}

QRectF
StageView::boundingRect() const
{ return cvt<PIXEL_SCALE,M>(bbox_meters_);}

void
StageView::paint(QPainter *p, const QStyleOptionGraphicsItem *o, QWidget *widget)
{ 
  p->setPen(pen_);
  p->setBrush(brush_);
  p->drawRect(cvt<PIXEL_SCALE,M>(bbox_meters_));
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
