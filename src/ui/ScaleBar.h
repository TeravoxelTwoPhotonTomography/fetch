#pragma once

#include <QtGui>
#include <QtOpenGL>

#define GOLDENRATIO 1.61803399

class ScaleBar: public QObject
{
  Q_OBJECT

public:
  ScaleBar(double         unit2px, 
           double         aspect    = 5.0*GOLDENRATIO, 
           QObject*       parent    = 0);

  QRectF boundingRect() const;
  void   paint       (QPainter* painter,const QRectF& rect);

  inline double scale() {return unit2px_;}
	
public slots:
  void setScale(double unit2px);
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
  double            unit2px_;       // The conversion form "unit" to pixels.
};
