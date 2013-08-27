#pragma once

#include <QtWidgets>
#include <ui/StageController.h>

namespace fetch {
namespace ui {

class StageView: public QGraphicsObject
{ Q_OBJECT

public:
  StageView(PlanarStageController *stageControl,QGraphicsItem *parent=0);
  virtual ~StageView();
  
  QRectF boundingRect  () const;                                           // in meters
  void   paint         (QPainter                       *painter,
                        const QStyleOptionGraphicsItem *option,
                        QWidget                        *widget = 0);

  virtual void mousePressEvent( QGraphicsSceneMouseEvent *event );

protected slots:
  void poll_stage();  // updates the view as the stage moves, stops the timer when the motion stops

private:
  QRectF  bbox_meters_;
  QPointF pos_meters_;

  QPen    pen_;
  QPen    target_pen_;
  QBrush  brush_;

  QTimer  stage_poll_timer_;

  PlanarStageController *control_;
};


}}//end fetch::ui


