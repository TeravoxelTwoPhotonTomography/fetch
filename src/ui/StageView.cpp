#include "ui/StageView.h"

namespace fetch {
namespace ui {

StageView::StageView(QGraphicsItem *parent)
        : QGraphicsItem(parent)
        , bbox_meters_(0,0,0.1,0.1)
        , pos_meters_(0.0,0.0)
        , pen_(Qt::white)
        , brush_(Qt::darkGray)
{ bbox_meters_.moveCenter(pos_meters_);
}

StageView::~StageView()
{
}

QRectF
StageView::boundingRect() const
{ return bbox_meters_;}

void
StageView::paint(QPainter *p, const QStyleOptionGraphicsItem *o, QWidget *widget)
{ p->setPen(pen_);
  p->setBrush(brush_);
  p->drawRect(bbox_meters_);
}

}} //end namespace fetch::ui