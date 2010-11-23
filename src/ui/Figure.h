#pragma once
#include <QtGui>
namespace mylib {
#include <array.h>
}
#include "imitem.h"

namespace fetch {
namespace ui {

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
private:
	QGraphicsView* _view;
	QGraphicsScene _scene;
	ImItem*        _item;
	
};


Figure* imshow (mylib::Array *im);
void    imclose(Figure *w=0);



}
}