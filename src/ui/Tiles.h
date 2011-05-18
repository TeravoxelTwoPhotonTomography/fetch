#pragma once

#include <QtGui>
#include <QtOpenGL>

namespace mylib
{
#include <array.h>
}


#define GOLDENRATIO 1.61803399

class TilesView : public QGraphicsObject
{
  Q_OBJECT

public:
  TilesView(QGraphicsItem* parent = 0);


  ~TilesView();

  QRectF boundingRect() const;
  void   paint         (QPainter                       *painter,
                        const QStyleOptionGraphicsItem *option,
                        QWidget                        *widget = 0);

public slots:
  void addSelection(const QPainterPath& path);
  void removeSelection(const QPainterPath& path);

protected:
  virtual void hoverMoveEvent (QGraphicsSceneHoverEvent *event);
  virtual void hoverLeaveEvent(QGraphicsSceneHoverEvent *event);

private:
  QGLBuffer vbo_,ibo_,cbo_;                    //vertex, index, and color  buffer objects for big lattice
  //QGLBUffer cursorColorVBO_,cursorIndexVBO_; //colors and indeces for highlighting tiles near the cursor
  //bool      cursorActive_;                   //
  //int       cursorIndexLength_;
  int       icursor_;
  QRectF    bbox_;

  QImage    *latticeImage_;

  void draw_grid_();
  void draw_cursor_();
  void init_cursor_(const QPointF& pos); 
  void initCBO();
  void updateCBO();
};