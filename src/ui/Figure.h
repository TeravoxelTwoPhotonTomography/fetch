#pragma once
#include <QtGui>
namespace mylib {
#include <array.h>
}
#include "imitem.h"
#include "ScaleBar.h"
#include "StageView.h"

namespace fetch {
namespace ui {

class ZoomableView:public QGraphicsView
{
	Q_OBJECT
public:
	ZoomableView(QGraphicsScene *scene, QWidget *parent=0);
	
  //inline double metersToPixels()                                           {return _scalebar.unit2px();}
  //inline void   setMetersToPixels(double unit2px)                          {_scalebar.setUnit(unit2px);}
	virtual void	wheelEvent(QWheelEvent *event);

  inline void notifyZoomChanged()                                            {emit zoomChanged(transform().m11());}
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
	Figure(PlanarStageController *stageController, QWidget *parent=0);
  Figure(double unit2px, QWidget *parent=0);
  virtual ~Figure();
	
public slots:
	inline void push(mylib::Array *im)                                       {_item->push(im); emit pushed(); maybeFit();}
	inline void imshow(mylib::Array *im)                                     {_item->push(im); _item->flip(); maybeFit(); }
  inline void fit(void)                                                    {_view->fitInView(_item->mapRectToScene(_item->boundingRect()),Qt::KeepAspectRatio);_view->notifyZoomChanged();}
  inline void fitNext(void)                                                {_isFitOnNext=true;}
  inline void updatePos(void)                                              {_item->setPos(units::cvt<units::PIXEL_SCALE,PlanarStageController::Unit>(_stage->pos()));}
  inline void updatePos(QPointF r)                                         {_item->setPos(units::cvt<units::PIXEL_SCALE,PlanarStageController::Unit>(r));}

  inline void setPixelSizeMicrons(double w, double h)                      {_item->setPixelSizeMicrons(w,h);}

signals:
	void lastFigureClosed();
  void pushed();

protected:
  void readSettings();
  void writeSettings();

private:
  inline void maybeFit()                                                   {if(_isFitOnNext) {fit(); _isFitOnNext=false;}}
private:
	ZoomableView*      _view;
	QGraphicsScene     _scene;
	ImItem*            _item;
  StageView*         _stage;

  bool _isFitOnNext;
};


//Figure* imshow (mylib::Array *im);
//void    imclose(Figure *w=0);

}} //end namespace fetch::ui
