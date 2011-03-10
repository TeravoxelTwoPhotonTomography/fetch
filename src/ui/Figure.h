#pragma once
#include <QtGui>
namespace mylib {
#include <array.h>
}
#include "imitem.h"
#include "ScaleBar.h"

namespace fetch {
namespace ui {

class ZoomableView:public QGraphicsView
{
	Q_OBJECT
public:
	ZoomableView(QGraphicsScene *scene, QWidget *parent=0);
	
	virtual void	wheelEvent(QWheelEvent *event);
signals:
	void zoomChanged(double zoom);
	
protected:
	virtual void drawForeground(QPainter* painter, const QRectF& rect);
	
private:
	ScaleBar _scalebar;
};


class Figure:public QWidget
{
  Q_OBJECT
public:
	Figure(QWidget *parent=0);
	
public slots:
	inline void push(mylib::Array *im)   {_item->push(im);}
	inline void imshow(mylib::Array *im) {_item->push(im); _item->flip(); _scene.setSceneRect(_scene.itemsBoundingRect());}
signals:
	void lastFigureClosed();
protected:
  void readSettings();
  void writeSettings();
  void closeEvent(QCloseEvent *event);
private:
	QGraphicsView*     _view;
	QGraphicsScene     _scene;
	ImItem*            _item;
};


Figure* imshow (mylib::Array *im);
void    imclose(Figure *w=0);

}} //end namespace fetch::ui