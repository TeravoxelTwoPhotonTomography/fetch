/** \file
    Histogram Dock Widget
    
    \todo never need to rescale cdf axis, always 0 to 1
    \todo options for scaling on x-axis...full range, zoom to range (and fix)
    /todo size x axis by cdf? e.g where cdf is between 0.05 to 0.95? 
    \todo inspection by hover over cdf
*/
#include "HistogramDockWidget.h"
#include "qcustomplot.h"
#include "common.h"
#include <cmath>  

namespace mylib {
#include "image.h"
}

#ifdef _MSC_VER
#define restrict __restrict
#else
#define restrict __restrict__ 
#endif

#define ENDL "\r\n"
#define PANIC(e) do{if(!(e)){qFatal("%s(%d)"ENDL "\tExpression evalatuated as false."ENDL "\t%s"ENDL,__FILE__,__LINE__,#e);           }}while(0)
#define FAIL     do{         qFatal("%s(%d)"ENDL "\tExecution should not reach here."ENDL,__FILE__,__LINE__);                                 }while(0)
#define TRY(e)   do{if(!(e)){qDebug("%s(%d)"ENDL "\tExpression evalatuated as false."ENDL "\t%s"ENDL,__FILE__,__LINE__,#e);goto Error;}}while(0)
#define HERE     qDebug("%s(%d). HERE."ENDL,__FILE__,__LINE__)

#define HINT_NBINS (1<<12)

namespace fetch{
namespace ui {

  HistogramDockWidget::HistogramDockWidget(QWidget *parent)
    : QDockWidget("Histogram",parent)
    , plot_(0)
    , ichan_(0)
    , last_(0)
    , x_(HINT_NBINS)
    , pdf_(HINT_NBINS)
    , cdf_(HINT_NBINS)
    , minX_(DBL_MAX)
    , maxX_(0)
    , perct_(0.5)
    , leMin_(0)
    , leMax_(0)
    , lePerct_(0)
  {
    QBoxLayout *layout = new QVBoxLayout;
    {
      QWidget *w = new QWidget(this);    
      w->setLayout(layout);
      setWidget(w);
    }

    QFormLayout *form = new QFormLayout;
    // channel selector
    { 
      QComboBox *c=new QComboBox;
      for(int i=0;i<4;++i) // initially add 4 channels to select
        c->addItem(QString("%1").arg(i),QVariant());
      PANIC(connect(   c,SIGNAL(currentIndexChanged(int)),
        this,SLOT(set_ichan(int))));
      PANIC(connect(this,SIGNAL(change_chan(int)),
        c,SLOT(setCurrentIndex(int))));
      form->addRow(tr("Channel:"),c);
    }

    { 
      QHBoxLayout *row = new QHBoxLayout();
      QPushButton *b;
      // Live update button
      b=new QPushButton("Live update");
      b->setCheckable(true);      
      PANIC(connect(  b,SIGNAL(toggled(bool)),
        this,SLOT(set_live(bool))));
      b->setChecked(true);
      row->addWidget(b);

      // Rescale button
      b=new QPushButton("Rescale");     
      PANIC(connect(   b,SIGNAL(pressed()),
        this,SLOT(rescale_axes())));
      b->setChecked(true);
      row->addWidget(b);
      form->addRow(row);
    }

    {
      QHBoxLayout *row = new QHBoxLayout();
      QPushButton *b;
      //percentile label
      QLabel *perctLabel = new QLabel(tr("Percentile:"));
      row->addWidget(perctLabel);
      ///// Line Edit Control - Gamma
      lePerct_= new QLineEdit("0.5");    
      QCompleter *c = new QCompleter(new QStringListModel(lePerct_),lePerct_);// Add completion based on history
      c->setCompletionMode(QCompleter::UnfilteredPopupCompletion);
      c->setModelSorting(QCompleter::UnsortedModel);
      lePerct_->setCompleter(c);
      lePerct_->setValidator(new QDoubleValidator(0.0001,1.0000,4,lePerct_));      // Add validated input
      PANIC(connect(lePerct_,SIGNAL(textEdited(QString)),
        this,  SLOT(set_percentile(QString))));
      row->addWidget(lePerct_);

      // Reset button
      b=new QPushButton("Reset");     
      PANIC(connect(   b,SIGNAL(pressed()),
        this,SLOT(reset_minmax())));
      row->addWidget(b);
      form->addRow(row);
    }

    {
      QHBoxLayout *row = new QHBoxLayout();
 
      QLabel *lbMin = new QLabel(tr("Minimum:"));
      row->addWidget(lbMin);
      ///// Line Edit Control - Minimum
      leMin_= new QLineEdit("DBL_MAX");
      leMin_->setReadOnly(true);
      row->addWidget(leMin_);

 
      QLabel *lbMax = new QLabel(tr("Maximum:"));
      row->addWidget(lbMax);
      ///// Line Edit Control - Maximum
      leMax_= new QLineEdit("0");
      leMax_->setReadOnly(true);;
      row->addWidget(leMax_);
      form->addRow(row);
    }

    layout->addLayout(form);

    // plot
    plot_=new QCustomPlot;
    plot_->setMinimumHeight(100);
    plot_->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
    plot_->addGraph();
    plot_->graph(0)->setPen(QPen(Qt::blue));
    plot_->addGraph(plot_->xAxis,plot_->yAxis2);
    plot_->graph(1)->setPen(QPen(Qt::red));
    plot_->xAxis->setLabel("Intensity");
    plot_->xAxis2->setVisible(true);
    plot_->yAxis->setLabel("PDF");
    plot_->yAxis2->setLabelColor(Qt::blue);
    plot_->yAxis2->setVisible(true);
    plot_->yAxis2->setLabel("CDF");
    plot_->yAxis2->setLabelColor(Qt::red);
    layout->addWidget(plot_);
  }
  
// histogram utilities START  
#define TYPECASES(ARRAYARG) do {\
    switch(ARRAYARG->type)  \
    { CASE( UINT8_TYPE ,u8); \
      CASE( UINT16_TYPE,u16);\
      CASE( UINT32_TYPE,u32);\
      CASE( UINT64_TYPE,u64);\
      CASE(  INT8_TYPE ,i8); \
      CASE(  INT16_TYPE,i16);\
      CASE(  INT32_TYPE,i32);\
      CASE(  INT64_TYPE,i64);\
      CASE(FLOAT32_TYPE,f32);\
      CASE(FLOAT64_TYPE,f64);\
      default: \
        FAIL;  \
    }}while(0)

  static double amin(mylib::Array *a)
  { double out;
#define CASE(ID,T) case mylib::ID: {const T *d=(T*)a->data; T m=d[0]; for(size_t i=1;i<a->size;++i) m=(d[i]<m)?d[i]:m; out=(double)m;} break;
    TYPECASES(a);
#undef CASE
    return out;
  }

  static double amax(mylib::Array *a)
  { double out;
#define CASE(ID,T) case mylib::ID: {const T *d=(T*)a->data; T m=d[0]; for(size_t i=1;i<a->size;++i) m=(d[i]>m)?d[i]:m; out=(double)m;} break;    
    TYPECASES(a);
#undef CASE
    return out;    
  }
  
  static unsigned nbins(mylib::Array *a,double min, double max)
  { unsigned n,lim = 1<<12; // max number of bins
    switch(a->type)
    { case mylib::UINT8_TYPE:
      case mylib::INT8_TYPE:
        lim=1<<8;
      case mylib::UINT16_TYPE:
      case mylib::UINT32_TYPE:
      case mylib::UINT64_TYPE:
      case mylib::INT16_TYPE:
      case mylib::INT32_TYPE:
      case mylib::INT64_TYPE:
        n=max-min+1;
        return (n<lim)?n:lim;      
      case mylib::FLOAT32_TYPE:
      case mylib::FLOAT64_TYPE:
        return lim;
      default:
        FAIL;
    }
  }
  
  static double binsize(unsigned n, double min, double max)
  { return (n==0)?0:(((double)n)-1)/(max-min);
  }
  
  template<class T>
  static void count(double *restrict pdf, size_t nbins, T *restrict data, size_t nelem, T min, double dy)
  { for(size_t i=0;i<nelem;++i) pdf[ (size_t)((data[i]-min)*dy) ]++;      
    for(size_t i=0;i<nbins;++i) pdf[i]/=(double)nelem;
  }
  static void scan(double *restrict cdf, double *restrict pdf, size_t n)
  { memcpy(cdf,pdf,n*sizeof(double));
    for(size_t i=1;i<n;++i) cdf[i]+=cdf[i-1];
  }
  static void setbins(double *x, size_t n, double min, double dy)
  { for(size_t i=0;i<n;++i) x[i] = i/dy+min;
  }

  static size_t findIndex(QVector<double> &cdf, double perct)
  {
    size_t index;
    double * cdfdata = cdf.data();
    double diff=1.0, minDiff = 1.0;

    for (size_t i=0; i<cdf.size(); ++i)
    {
      diff = abs(cdfdata[i] - perct);
      if (diff <= minDiff) 
      {
        minDiff = diff;
        index = i;
      }
    }

    return index;
  }
      
  static void histogram(QVector<double> &x, QVector<double> &pdf, QVector<double> &cdf, mylib::Array *a)
  { double min,max,dy;
    unsigned n;
    min=amin(a);
    max=amax(a);
#if 0
    HERE;
    mylib::Write_Image("histogram.tif",a,mylib::DONT_PRESS);    
#endif
//    debug("%s(%d) %s"ENDL "\tmin %6.1f\tmax %6.1f"ENDL,__FILE__,__LINE__,__FUNCTION__,min,max);
    n=nbins(a,min,max);
    dy=binsize(n,min,max); // value transform is  (data[i]-min)*dy --> for max this is (max-min)*(nbins-1)/(max-min)

    x.resize(n);
    pdf.resize(n);
    cdf.resize(n);
    
#define CASE(ID,T) case mylib::ID: count<T>(pdf.data(),n,(T*)a->data,a->size,min,dy); break;
    TYPECASES(a);
#undef CASE
    scan(cdf.data(),pdf.data(),n);
    setbins(x.data(),n,min,dy);
  }
 // histogram utilities END

 void HistogramDockWidget::set(mylib::Array *im)
  { if(!is_live_) return;
    TRY(check_chan(im));
    swap(im);
    compute(im);
 Error:
    ; // bad input, ignore 
  }

static int g_inited=0;

  /** Presumes channels are on different planes */
 void HistogramDockWidget::compute(mylib::Array *im)
  { 
    mylib::Array t,*ch;    
    QString tempStr;
    TRY(ch=mylib::Get_Array_Plane(&(t=*im),ichan_)); //select channel    
    histogram(x_,pdf_,cdf_,ch);
    plot_->graph(0)->setData(x_,pdf_);
    plot_->graph(1)->setData(x_,cdf_);
    if(!g_inited)
    { plot_->graph(0)->rescaleAxes();
      plot_->graph(1)->rescaleAxes();
      g_inited=1;
    }
    plot_->replot(); 

    //find intensity value for the input CDF percentile 
  int index;
  index = findIndex(cdf_, perct_);
  double xValue;
  xValue = x_.data()[index];
  if (xValue < minX_)  minX_ = xValue;
  if (xValue > maxX_)  maxX_ = xValue;
  
  tempStr.setNum(minX_);
  leMin_->setText(tempStr);
  tempStr.setNum(maxX_);
  leMax_->setText(tempStr);

 Error:
  ; // memory error or oob channel, should never get here.    
  }

 void HistogramDockWidget::rescale_axes()
  { plot_->graph(0)->rescaleAxes();
    plot_->graph(1)->rescaleAxes();
  }

void HistogramDockWidget::set_ichan(int ichan)
  { ichan_=ichan;   
    if(last_)
    { check_chan(last_);
      compute(last_);
    }
  }

void HistogramDockWidget::set_live(bool is_live)
  { is_live_=is_live;
  }

void HistogramDockWidget::reset_minmax()
  {
    minX_ = DBL_MAX;
    maxX_ = 0;
  }

void HistogramDockWidget::set_percentile(QString text)
  {
    perct_ = text.toDouble();
    reset_minmax();
  }

  /** updates the last_ array with a new one. */
void HistogramDockWidget::swap(mylib::Array *im)
  { 
    mylib::Array *t=last_;
    TRY(last_=mylib::Copy_Array(im));
    if(t) mylib::Free_Array(t);
  Error:
    ; // presumably a memory error...not sure what to do, should be rare    
  }

 int HistogramDockWidget::check_chan(mylib::Array *im)
  { int oc=ichan_;
    TRY(im->ndims<=3);
    if( (im->ndims==2) && (ichan_!=0) )
      ichan_=0;
    if( (im->ndims==3) && (ichan_>=im->dims[2]) )
      ichan_=0;
    if(ichan_!=oc)
      emit change_chan(ichan_);
    return 1;
Error:
    return 0;      
  }

}} //namspace fetch::ui
