#pragma once

#include <QtGui>

namespace fetch {
namespace ui {

class StageView: public QGraphicsItem
{
public:
  StageView(QGraphicsItem *parent=0);
  virtual ~StageView();
	
  QRectF boundingRect  () const;                                           // in meters
  void   paint         (QPainter                       *painter,
                        const QStyleOptionGraphicsItem *option,
                        QWidget                        *widget = 0);

private:
  QRectF  bbox_meters_;
  QPointF pos_meters_;

  QPen    pen_;
  QBrush  brush_;
};


}}//end fetch::ui


