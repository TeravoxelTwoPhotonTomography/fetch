//#ifdef _WIN32
//#include <GL/glew.h>
//#endif
#include "ScaleBar.h"
#include "units.h"
#include <QString>

using namespace fetch::units;


#define HERE printf("[ScaleBar] At %s(%d)\n",__FILE__,__LINE__)


struct UnitData
{ int       mag_;
  QString   lbl_;
  UnitData(int mag, QString lbl) : mag_(mag), lbl_(lbl) {qDebug()<<lbl.size();}
  UnitData(int mag, const char* lbl) : mag_(mag), lbl_() {lbl_=QString::fromUtf8(lbl);}
  UnitData(int mag, const wchar_t* lbl) : mag_(mag), lbl_() 
  { lbl_=QString::fromUtf16((const ushort*) lbl);
  }
};

static struct UnitData UnitTable[] = 
{ UnitData(-12,"pm"),
  UnitData( -9,"nm"),
  UnitData( -6,"µm"),
  UnitData( -3,"mm"),
  UnitData( -2,"cm"),
  UnitData(  0,"m"),
  UnitData(  3,"km")
};

static int findTextUnit(double width, double* display_width)
{ double mag = log10(width),
         argmin;
  unsigned i,best;
  static const unsigned nelem = sizeof(UnitTable)/sizeof(UnitData);
  
  best   = -1;
  argmin = 100;
  if (mag<UnitTable[0].mag_) {
    best = 0;
  } else if (mag>UnitTable[nelem-1].mag_) {
    best = nelem-1;
  } else
  {
    for(i=0;i<nelem;++i)
    { double d = mag - UnitTable[i].mag_; //-2.2 - (-3) = 0.8, //-2.2 - (-2) = -0.2 
      if( d>=0.0 && d<argmin)
      { argmin=d;
        best = i;
      }
    }
  }
  *display_width = pow(10.0,mag-UnitTable[best].mag_);
  return best;
}

static inline double suggestWidth(double scale,double aspect,double hpx)
{ //computes width=10^i for int i that gives aspect closest to target
  return pow(10.0,floor(0.5+(log10(aspect*hpx/scale))));
}

ScaleBar::ScaleBar(double aspect, double zoom, QObject *parent)
  : QObject(parent)
  , text_("Not set")
  , text_pos_()
  , rect_()
  , aspect_(aspect)
  , width_units_(0.0)
  , scale_(zoom)
{ 
  
  //text_.setDefaultTextColor(QColor(255,255,255,255));
  font_.setBold(true);
  font_.setPointSize(24);
  text_.prepare(QTransform(),font_);
  //rect_.setPen(QColor(0,0,0,255));
  //rect_.setBrush(QColor(255,255,255,255));
  setZoom(zoom);  
}

QRectF
ScaleBar::boundingRect() const
{ QRectF r(text_pos_,text_.size());
  return rect_.united(r);
}

void
ScaleBar::paint(QPainter *painter, const QRectF& rect)
{

  painter->setPen(QColor(Qt::black));
  painter->setBrush(QColor(Qt::white));
  painter->drawRect(rect_);
  painter->setPen(QColor(Qt::white));
  painter->setBrush(QColor(Qt::white));
  painter->setFont(font_);
  painter->drawStaticText(text_pos_, text_);

  //draw static text will leave a texture bound. unbind it
  //glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_2D,0);

#if 0
  painter->setRenderHint(QPainter::Antialiasing, true);
  painter->setRenderHint(QPainter::HighQualityAntialiasing, true);
  painter->setRenderHint(QPainter::TextAntialiasing, true);

  painter->setPen(QColor(Qt::red));
  painter->setBrush(QColor(Qt::red));
  painter->drawEllipse(text_pos_,10,10);
  painter->setBrush(QColor(Qt::transparent));
  QRectF r(text_pos_,text_.size());
  painter->drawRect(r);
  painter->setPen(QColor(Qt::yellow));
  painter->setBrush(QColor(Qt::yellow));
  painter->drawEllipse(rect_.topLeft(),5,5);
  painter->setBrush(QColor(Qt::transparent));
  painter->drawRect(rect_);
#endif
}

void
ScaleBar::setZoom(double scale)
{ int hpx;
  QFontMetrics fm(font_);
  hpx = fm.height()/2;
  
  double disp;
  double w = suggestWidth(cvt<PIXEL_SCALE,M>(scale),aspect_,hpx);
  int itxt = findTextUnit(w,&disp);
  
#if 0
  qDebug() << " *** SCALE ---";	
  qDebug() << " *** SCALE ---     zoom: " << scale;  
  qDebug() << " *** SCALE ---        w: " << w;
  qDebug() << " *** SCALE ---     itxt: " << itxt;
  qDebug() << " *** SCALE ---";
#endif
  
  if(itxt<0) return; // none found, so don't change
  
  //unit2px_ = unit2px;
  scale_   = scale;
  width_units_ = cvt<PIXEL_SCALE,M>(w);
  text_.setText(QString("%1 %2").arg((int)disp).arg(UnitTable[itxt].lbl_));
  rect_.setRect(0,0,width_units_*scale_,hpx);
  QRectF r(text_pos_,text_.size());
  r.moveCenter(rect_.center());
  r.moveLeft(rect_.right());
  text_pos_ = r.topLeft();

  return;
}