#include<ui/DevicePropController.h>
#include<types.h>
#include "common.h"
#include <google\protobuf\descriptor.h>

namespace fetch {
namespace ui {

#define WRN(expr) if(!(expr)) warning("%s(%d) Expression evaluated to false"ENDL"\t%s"ENDL,__FILE__,__LINE__,#expr)

#define CVT(T,FN)   \
    *ok=0;          \
    T v = s.FN(ok);  \
    if(!(*ok)) error("Couldn't interpret contents of line edit control.\r\n" \
                     "\tA QValidator should be attached to avoid this problem.\r\n"); \
    return v


template<> u8  QStringToValue<u8 >(QString &s,bool *ok) { CVT(u8 ,toUShort);}
template<> u16 QStringToValue<u16>(QString &s,bool *ok) { CVT(u16,toUShort);}
template<> u32 QStringToValue<u32>(QString &s,bool *ok) { CVT(u32,toUInt);}
template<> u64 QStringToValue<u64>(QString &s,bool *ok) { CVT(u64,toULongLong);}
template<> i8  QStringToValue<i8 >(QString &s,bool *ok) { CVT(i8 ,toShort);}
template<> i16 QStringToValue<i16>(QString &s,bool *ok) { CVT(i16,toShort);}
template<> i32 QStringToValue<i32>(QString &s,bool *ok) { CVT(i32,toInt);}
template<> i64 QStringToValue<i64>(QString &s,bool *ok) { CVT(i64,toLongLong);}
template<> f32 QStringToValue<f32>(QString &s,bool *ok) { CVT(f32,toFloat );}
template<> f64 QStringToValue<f64>(QString &s,bool *ok) { CVT(f64,toDouble);}

template<> QString ValueToQString(u8  v) {return QString().setNum(v);}
template<> QString ValueToQString(u16 v) {return QString().setNum(v);}
template<> QString ValueToQString(u32 v) {return QString().setNum(v);}
template<> QString ValueToQString(u64 v) {return QString().setNum(v);}
template<> QString ValueToQString(i8  v) {return QString().setNum(v);}
template<> QString ValueToQString(i16 v) {return QString().setNum(v);}
template<> QString ValueToQString(i32 v) {return QString().setNum(v);}
template<> QString ValueToQString(i64 v) {return QString().setNum(v);}
template<> QString ValueToQString(f32 v) {return QString().setNum(v);}
template<> QString ValueToQString(f64 v) {return QString().setNum(v);}

/* Note:
   This is an ugly little set of convrsion functions.  Ideally, the non-trivial one's would never be used.
   These help me make polymorphic controllers (that generate a QDoubleSpinBox).
   Add more on demand.
*/
template<> double doubleToValue(double v){return v;}
template<> float  doubleToValue(double v){return v;}
template<> int doubleToValue(double v){return (int)v;}
template<> unsigned int doubleToValue(double v){return (unsigned int)v;}
template<> cfg::device::Vibratome::VibratomeFeedAxis doubleToValue(double v) { return (v>0.5)?cfg::device::Vibratome_VibratomeFeedAxis_X:cfg::device::Vibratome_VibratomeFeedAxis_Y;}

void DevicePropControllerBase::report() 
{ 
  if(le_)
  { QStringListModel *model = qobject_cast<QStringListModel*>(le_->completer()->model());
    if(model)
    { QStringList hist = model->stringList();
      hist.append(le_->text());
      hist.removeDuplicates();
      model->setStringList(hist);
    }
  }
}

//////////////////////////////////////////////////////////////////////////
// Auxiliary QValidators
//////////////////////////////////////////////////////////////////////////

class EvenIntValidator:public QIntValidator
{
public:
  EvenIntValidator(int min, int max, QObject *parent) : QIntValidator(min,max,parent) {}
  virtual State validate(QString &in, int &pos) const
  {
    State state = QIntValidator::validate(in,pos);
    if(state!=Acceptable)
      return state;
    else {
      bool ok;
      unsigned v = in.toUInt(&ok);
      if(ok)
        if(v%2)
          return QValidator::Intermediate;
        else
          return state;
      return QValidator::Invalid;
    }
  }
  virtual void fixup(QString &input) const
  {
    bool ok;
    unsigned v = input.toUInt(&ok);
    if(ok)
    {
      if(v%2) // odd
        input = QString().setNum(v+1);
    }
  }
};

//////////////////////////////////////////////////////////////////////////
// Getters and Setters 
////////////////////////////////////////////////////////////////////////// 

// Video

void GetSetResonantTurn::Set_(device::Microscope *dc, f64 &v)
{ device::Microscope::Config c = dc->get_config();
  // DEPRECATED c.mutable_resonant_wrap()->set_turn_px(v);
  Guarded_Assert(dc->set_config_nowait(c));
}
f64 GetSetResonantTurn::Get_(device::Microscope *dc)
{  return 0.0; // DEPRECATED c.resonant_wrap().turn_px();
}
QValidator* GetSetResonantTurn::createValidator_(QObject* parent)
{ return new QDoubleValidator (0.0, 2048.0, 1, parent);
}

void GetSetLines::Set_(device::Microscope *dc, i32 &v)
{ device::Microscope::Config c = dc->get_config();
  c.mutable_scanner3d()->mutable_scanner2d()->set_nscans(v/2);
  Guarded_Assert(dc->set_config_nowait(c));
}
i32 GetSetLines::Get_(device::Microscope *dc)
{ device::Microscope::Config c = dc->get_config(); 
  return 2*c.scanner3d().scanner2d().nscans();
}          
QValidator* GetSetLines::createValidator_(QObject* parent)
{ return new EvenIntValidator(2,4096,parent);
}

void GetSetLSMVerticalRange::Set_(device::LinearScanMirror *dc, f64 &v)
{ dc->setAmplitudeVolts(v);
}
f64 GetSetLSMVerticalRange::Get_(device::LinearScanMirror *dc)
{ return dc->getAmplitudeVolts();
}        
QValidator* GetSetLSMVerticalRange::createValidator_(QObject* parent)
{ return new QDoubleValidator(0.0,20.0,3,parent);
}

void GetSetPockels::Set_(device::Pockels *dc, u32 &v)
{ dc->setOpenPercentNoWait(v); // convert mV to V
}
u32 GetSetPockels::Get_(device::Pockels *dc)
{ return floor(0.5+dc->getOpenPercent()); // round
}           
QValidator* GetSetPockels::createValidator_(QObject* parent)
{ return new QIntValidator(0,100,parent);
}

// Vibratome

void GetSetVibratomeAmplitude::Set_(device::Vibratome *dc, u32 &v)
{ dc->setAmplitude(v);
}
u32 GetSetVibratomeAmplitude::Get_(device::Vibratome *dc)
{ return dc->getAmplitude_ControllerUnits();
}
QValidator* GetSetVibratomeAmplitude::createValidator_(QObject* parent)
{ return new QIntValidator(0,255,parent);
}

void GetSetVibratomeFeedDist::Set_(device::Vibratome *dc, double &v)
{ dc->setFeedDist_mm(v);
}
double GetSetVibratomeFeedDist::Get_(device::Vibratome *dc)
{ return dc->getFeedDist_mm();
}
QValidator* GetSetVibratomeFeedDist::createValidator_(QObject* parent)
{ //should check against stage bounds
  return new QDoubleValidator(-100.0,100.0/*mm*/,4/*decimals*/,parent);
}     

void GetSetVibratomeFeedVel::Set_(device::Vibratome *dc, double &v)
{ dc->setFeedVel_mm(v);
}
double GetSetVibratomeFeedVel::Get_(device::Vibratome *dc)
{ return dc->getFeedVel_mm();
}
QValidator* GetSetVibratomeFeedVel::createValidator_(QObject* parent)
{ //should check against stage bounds
  return new QDoubleValidator(0.0,10.0/*mm/sec*/,4/*decimals*/,parent);
}
     
void GetSetVibratomeCutPosX::Set_(device::Vibratome *dc, double &v)
{ float x,y;
  dc->feed_begin_pos_mm(&x,&y);
  dc->set_feed_begin_pos_mm(v,y);  
}
double GetSetVibratomeCutPosX::Get_(device::Vibratome *dc)
{ float x,y;
  dc->feed_begin_pos_mm(&x,&y);  
  return x;
}
QValidator* GetSetVibratomeCutPosX::createValidator_(QObject* parent)
{ //should check against stage bounds
  return new QDoubleValidator(0.0,10.0/*mm*/,4/*decimals*/,parent);
}
     
void GetSetVibratomeCutPosY::Set_(device::Vibratome *dc, double &v)
{ float x,y;
  dc->feed_begin_pos_mm(&x,&y);
  dc->set_feed_begin_pos_mm(x,v);  
}
double GetSetVibratomeCutPosY::Get_(device::Vibratome *dc)
{ float x,y;
  dc->feed_begin_pos_mm(&x,&y);  
  return y;
}
QValidator* GetSetVibratomeCutPosY::createValidator_(QObject* parent)
{ //should check against stage bounds
  return new QDoubleValidator(0.0,10.0/*mm*/,4/*decimals*/,parent);
}

void 
  GetSetVibratomeFeedAxis::
  Set_(device::Vibratome *dc, cfg::device::Vibratome::VibratomeFeedAxis &v)
{ dc->setFeedAxis(v);
}
cfg::device::Vibratome::VibratomeFeedAxis 
  GetSetVibratomeFeedAxis::
  Get_(device::Vibratome *dc)
{ return dc->getFeedAxis();
}
QValidator* 
  GetSetVibratomeFeedAxis::
  createValidator_(QObject* parent)
{ return NULL;
}
const ::google::protobuf::EnumDescriptor* 
  GetSetVibratomeFeedAxis::
  enum_descriptor(device::Vibratome *dc)
{ return dc->_config->VibratomeFeedAxis_descriptor();
}
template<>
cfg::device::Vibratome_VibratomeFeedAxis
  QStringToValue<cfg::device::Vibratome_VibratomeFeedAxis>(QString &s, bool *ok)
{ cfg::device::Vibratome t;
  const ::google::protobuf::EnumDescriptor* d = t.VibratomeFeedAxis_descriptor();
  *ok = false;
  for(int i=0;i<d->value_count();++i)
    if(s.compare(d->value(i)->name().c_str(),Qt::CaseInsensitive)==0)
    { *ok = true;
      return (cfg::device::Vibratome_VibratomeFeedAxis)(d->value(i)->number());
    }
  return (cfg::device::Vibratome_VibratomeFeedAxis)(d->value(0)->number());
}
template<>
QString ValueToQString<cfg::device::Vibratome_VibratomeFeedAxis>(cfg::device::Vibratome_VibratomeFeedAxis v)
{ cfg::device::Vibratome t;
  const ::google::protobuf::EnumDescriptor* d = t.VibratomeFeedAxis_descriptor();
  for(int i=0;i<d->value_count();++i)
    if(d->value(i)->number()==(int)v)
      return d->value(i)->name().c_str();
  return QString();
}
    
void GetSetVibratomeZOffset::Set_(device::Vibratome *dc, float &v)
{ dc->setVerticalOffsetNoWait(v); 
}
float GetSetVibratomeZOffset::Get_(device::Vibratome *dc)
{ return dc->verticalOffset();
}
QValidator* GetSetVibratomeZOffset::createValidator_(QObject* parent)
{ return new QDoubleValidator(-10.0,10.0/*mm*/,4/*decimals*/,parent);
}
    
void GetSetVibratomeThick::Set_(device::Vibratome *dc, float &v)
{ dc->setThicknessUmNoWait(v); 
}
float GetSetVibratomeThick::Get_(device::Vibratome *dc)
{ return dc->thickness_um();
}
QValidator* GetSetVibratomeThick::createValidator_(QObject* parent)
{ return new QDoubleValidator(0.0,1000.0/*um*/,4/*decimals*/,parent);
}

// Stage

void GetSetStagePosX::Set_(device::Stage *dc, float &v)
{ float x,y,z;
  dc->getTarget(&x,&y,&z);
  dc->setPosNoWait(v,y,z);
}
float GetSetStagePosX::Get_(device::Stage *dc)
{ float x,y,z;
  dc->getTarget(&x,&y,&z);
  return x;
}
QValidator* GetSetStagePosX::createValidator_(QObject* parent)
{ return new QDoubleValidator (0.0, 10.0, 1, parent);
}

void GetSetStagePosY::Set_(device::Stage *dc, float &v)
{ float x,y,z;
  dc->getTarget(&x,&y,&z);
  dc->setPosNoWait(x,v,z);
}
float GetSetStagePosY::Get_(device::Stage *dc)
{ float x,y,z;
  dc->getTarget(&x,&y,&z);
  return y;
}
QValidator* GetSetStagePosY::createValidator_(QObject* parent)
{ return new QDoubleValidator (0.0, 10.0, 1, parent);
} 

void GetSetStagePosZ::Set_(device::Stage *dc, float &v)
{ float x,y,z;
  dc->getTarget(&x,&y,&z);
  dc->setPosNoWait(x,y,v);
}
float GetSetStagePosZ::Get_(device::Stage *dc)
{ float x,y,z;
  dc->getTarget(&x,&y,&z);
  return z;
}
QValidator* GetSetStagePosZ::createValidator_(QObject* parent)
{ return new QDoubleValidator (0.0, 10.0, 1, parent);
}

void GetSetStageVelX::Set_(device::Stage *dc, float &v)
{ float x,y,z;
  dc->getVelocity(&x,&y,&z);
  dc->setVelocity(v,y,z);
  
  device::Stage::Config cfg = dc->get_config();
  cfg::device::Point3d *p = cfg.mutable_default_velocity_mm_per_sec();
  p->set_x(v);
  WRN(dc->set_config_nowait(cfg));
}
float GetSetStageVelX::Get_(device::Stage *dc)
{ return dc->get_config().default_velocity_mm_per_sec().x();
}
QValidator* GetSetStageVelX::createValidator_(QObject* parent)
{ return new QDoubleValidator (0.0, 10.0, 1, parent);
}

void GetSetStageVelY::Set_(device::Stage *dc, float &v)
{ float x,y,z;
  dc->getVelocity(&x,&y,&z);
  dc->setVelocity(x,v,z);
  
  device::Stage::Config cfg = dc->get_config();
  cfg::device::Point3d *p = cfg.mutable_default_velocity_mm_per_sec();  
  p->set_y(v);  
  WRN(dc->set_config_nowait(cfg));
}
float GetSetStageVelY::Get_(device::Stage *dc)
{ return dc->get_config().default_velocity_mm_per_sec().y();
}
QValidator* GetSetStageVelY::createValidator_(QObject* parent)
{ return new QDoubleValidator (0.0, 10.0, 1, parent);
} 

void GetSetStageVelZ::Set_(device::Stage *dc, float &v)
{ float x,y,z;
  dc->getVelocity(&x,&y,&z);
  dc->setVelocity(x,y,v);
  
  device::Stage::Config cfg = dc->get_config();
  cfg::device::Point3d *p = cfg.mutable_default_velocity_mm_per_sec();  
  p->set_z(v);  
  WRN(dc->set_config_nowait(cfg));
}
float GetSetStageVelZ::Get_(device::Stage *dc)
{ return dc->get_config().default_velocity_mm_per_sec().z();
}
QValidator* GetSetStageVelZ::createValidator_(QObject* parent)
{ return new QDoubleValidator (0.0, 10.0, 1, parent);
}

// Stack

void GetSetZPiezoMin::Set_(device::ZPiezo *dc, f64 &v)
{ dc->setMin(v);
}
f64 GetSetZPiezoMin::Get_(device::ZPiezo *dc)
{ return dc->getMin();
}
QValidator* GetSetZPiezoMin::createValidator_(QObject* parent)
{ return new QDoubleValidator (0.0, 400.0, 1, parent);
}

void GetSetZPiezoMax::Set_(device::ZPiezo *dc, f64 &v)
{ dc->setMax(v);
}
f64 GetSetZPiezoMax::Get_(device::ZPiezo *dc)
{ return dc->getMax();
}
QValidator* GetSetZPiezoMax::createValidator_(QObject* parent)
{ return new QDoubleValidator (0.0, 400.0, 1, parent);
}

void GetSetZPiezoStep::Set_(device::ZPiezo *dc, f64 &v)
{ dc->setStep(v);
}
f64 GetSetZPiezoStep::Get_(device::ZPiezo *dc)
{ return dc->getStep();
}
QValidator* GetSetZPiezoStep::createValidator_(QObject* parent)
{ return new QDoubleValidator (0.0, 400.0, 3, parent);
}

// Field of View

void GetSetOverlapZ::Set_(device::FieldOfViewGeometry *dc, float &v)
{ dc->overlap_um_[2] = v;
}
float GetSetOverlapZ::Get_(device::FieldOfViewGeometry *dc)
{ return dc->overlap_um_[2];
}
QValidator* GetSetOverlapZ::createValidator_(QObject* parent)
{ return new QDoubleValidator (-400.0, 400.0, 1, parent);
}

// AutoTile
void GetSetAutoTileZOff::Set_(device::Microscope *dc, float &v)
{ device::Microscope::Config c = dc->get_config();
  c.mutable_autotile()->set_z_um(v);
  dc->set_config(c);
}
float GetSetAutoTileZOff::Get_(device::Microscope *dc)
{ return dc->get_config().autotile().z_um();
}
QValidator* GetSetAutoTileZOff::createValidator_(QObject* parent)
{ return new QDoubleValidator (0.0, 400.0, 1, parent);
}

void GetSetAutoTileZMax::Set_(device::Microscope *dc, float &v)
{ device::Microscope::Config c = dc->get_config();
  c.mutable_autotile()->set_maxz_mm(v);
  dc->set_config(c);
}
float GetSetAutoTileZMax::Get_(device::Microscope *dc)
{ return dc->get_config().autotile().maxz_mm();
}
QValidator* GetSetAutoTileZMax::createValidator_(QObject* parent)
{ return new QDoubleValidator (0.0, 12.5, 1, parent);
}

void GetSetAutoTileTimeoutMs::Set_(device::Microscope *dc, unsigned &v)
{ device::Microscope::Config c = dc->get_config();
  c.mutable_autotile()->set_timeout_ms(v);
  dc->set_config(c);
}
unsigned GetSetAutoTileTimeoutMs::Get_(device::Microscope *dc)
{ return dc->get_config().autotile().timeout_ms();
}
QValidator* GetSetAutoTileTimeoutMs::createValidator_(QObject* parent)
{ return new QIntValidator (0, 3600000, parent);
}   

void GetSetAutoTileChan::Set_(device::Microscope *dc, unsigned &v)
{ device::Microscope::Config c = dc->get_config();
  c.mutable_autotile()->set_ichan(v);
  dc->set_config(c);
}
unsigned GetSetAutoTileChan::Get_(device::Microscope *dc)
{ return dc->get_config().autotile().ichan();
}
QValidator* GetSetAutoTileChan::createValidator_(QObject* parent)
{ return new QIntValidator (0, 3, parent);
}

void GetSetAutoTileIntesityThreshold::Set_(device::Microscope *dc, float &v)
{ device::Microscope::Config c = dc->get_config();
  c.mutable_autotile()->set_intensity_threshold(v);
  dc->set_config(c);
}
float GetSetAutoTileIntesityThreshold::Get_(device::Microscope *dc)
{ return dc->get_config().autotile().intensity_threshold();
}
QValidator* GetSetAutoTileIntesityThreshold::createValidator_(QObject* parent)
{ return new QDoubleValidator(-5e9,5e9,1,parent);
}

void GetSetAutoTileAreaThreshold::Set_(device::Microscope *dc, float &v)
{ device::Microscope::Config c = dc->get_config();
  c.mutable_autotile()->set_area_threshold(v);
  dc->set_config(c);
}
float GetSetAutoTileAreaThreshold::Get_(device::Microscope *dc)
{ return dc->get_config().autotile().area_threshold();
}
QValidator* GetSetAutoTileAreaThreshold::createValidator_(QObject* parent)
{ return new QDoubleValidator(0.0, 1.0, 3, parent);
}

}} //end fetch::ui
