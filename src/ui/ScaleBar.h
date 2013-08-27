#pragma once

#include <QtWidgets>
#include <QtOpenGL>

#define GOLDENRATIO 1.61803399

class ScaleBar: public QObject
{
  Q_OBJECT

public:
  ScaleBar(double         aspect    = 5.0*GOLDENRATIO, 
           double         zoom      = 1.0,
           QObject*       parent    = 0);

  QRectF boundingRect() const;
  void   paint       (QPainter* painter,const QRectF& rect);
	
public slots:  
	void setZoom(double zoom);
  
signals:
  void scaleChanged();

private:
  QString           unit_;          // The "base" of the unit.  For um,nm,etc.. it would be 'm'
  QStaticText       text_;          // The text that gets shown next to the scale bar (e.g. 50 um)
	QFont							font_;
	QPointF           text_pos_;      // position of text origin relative to rect_ origin
  QRectF            rect_;          // The "bar" part of the scale bar.
  double            aspect_;
  double            width_units_;   // The width of the scale bar in "units."
  
  double            scale_;         // The view's scale - The conversion from "unit" to pixels.
};
