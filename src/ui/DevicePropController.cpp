#include<ui/DevicePropController.h>
#include<types.h>
#include "common.h"

namespace fetch {
namespace ui {

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
  c.mutable_resonant_wrap()->set_turn_px(v);
  Guarded_Assert(dc->set_config_nowait(c));
}
f64 GetSetResonantTurn::Get_(device::Microscope *dc)
{ device::Microscope::Config c = dc->get_config(); 
  return c.resonant_wrap().turn_px();
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
{ dc->setOpenVoltsNoWait(v/1000.0);
}
u32 GetSetPockels::Get_(device::Pockels *dc)
{ return dc->getOpenVolts();
}           
QValidator* GetSetPockels::createValidator_(QObject* parent)
{ return new QIntValidator(0,2000,parent);
}

void GetSetVibratomeAmplitude::Set_(device::Vibratome *dc, u32 &v)
{ dc->setAmplitude(v);
}
u32 GetSetVibratomeAmplitude::Get_(device::Vibratome *dc)
{ return dc->getAmplitude_ControllerUnits();
}
QValidator* GetSetVibratomeAmplitude::createValidator_(QObject* parent)
{ return new QIntValidator(0,255,parent);
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

}} //end fetch::ui
