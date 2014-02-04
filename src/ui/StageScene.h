#pragma once
#include <QtWidgets>

class SelectionRectGraphicsWidget : public QGraphicsWidget
{
  Q_OBJECT

public:
  enum RectOp {Add,Remove,Done,UnDone,Explorable,UnExplorable,Safe,UnSafe,Reset};

  SelectionRectGraphicsWidget(QGraphicsItem *parent=0);

  void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = 0 );

public slots:

  void setOpDone()         {lastop_=op_=Done; update();}
  void setOpUnDone()       {lastop_=op_=UnDone; update();}
  void setOpAdd()          {lastop_=op_=Add; update();}
  void setOpRemove()       {lastop_=op_=Remove; update();}
  void setOpExplorable()   {lastop_=op_=Explorable; update();}
  void setOpUnExplorable() {lastop_=op_=UnExplorable; update();}
  void setOpSafe()         {lastop_=op_=Safe; update();}
  void setOpUnSafe()       {lastop_=op_=UnSafe; update();}
  void setOpUserReset()    {lastop_=op_=Reset; update();}
  void commit();
  void cancel();

signals:
  void addSelectedArea(const QPainterPath& path);
  void removeSelectedArea(const QPainterPath& path);
  void markSelectedAreaAsDone(const QPainterPath& path);
  void markSelectedAreaAsNotDone(const QPainterPath& path);  
  void markSelectedAreaAsExplorable(const QPainterPath& path);
  void markSelectedAreaAsNotExplorable(const QPainterPath& path);  
  void markSelectedAreaAsSafe(const QPainterPath& path);
  void markSelectedAreaAsNotSafe(const QPainterPath& path);
  void markSelectedAreaUserReset(const QPainterPath& path);

protected:
  virtual void contextMenuEvent(QGraphicsSceneContextMenuEvent *event);

  virtual void focusInEvent(QFocusEvent *event);
  virtual void focusOutEvent(QFocusEvent *event);

private:
  RectOp op_;
  static RectOp lastop_;

  void createActions();

  QColor getPenColor();
  QColor getBaseColor();
};


class StageScene : public QGraphicsScene
{
  Q_OBJECT

public:
  enum Mode {DrawRect, Pick, DoNothing};  

  StageScene(QObject *parent=0);

  virtual void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
  virtual void mousePressEvent(QGraphicsSceneMouseEvent *event);
  virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);

public slots:
  void setMode(Mode mode);

signals:    
  void addSelectedArea(const QPainterPath& path);
  void removeSelectedArea(const QPainterPath& path);  
  void markSelectedAreaAsDone(const QPainterPath& path);
  void markSelectedAreaAsNotDone(const QPainterPath& path);
  void markSelectedAreaAsExplorable(const QPainterPath& path);
  void markSelectedAreaAsNotExplorable(const QPainterPath& path);
  void markSelectedAreaAsSafe(const QPainterPath& path);
  void markSelectedAreaAsNotSafe(const QPainterPath& path);
  void markSelectedAreaUserReset(const QPainterPath& path);

private:
  QGraphicsWidget *current_item_;
  Mode mode_;

};