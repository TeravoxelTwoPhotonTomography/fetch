#include<ui/WorkerController.h>
#include<types.h>

namespace fetch {
namespace ui {

#define CVT(T,FN)   \
    *ok=0;          \
    T v = FN(ok);  \
    if(!(*ok)) error("Couldn't interpret contents of line edit control.\r\n" \
                     "\tA QValidator should be attached to avoid this problem.\r\n"); \
    return v


template<u8 > u8  QStringToValue(QString &s,bool *ok) { CVT(u8 ,s.toUShort);}
template<u16> u16 QStringToValue(QString &s,bool *ok) { CVT(u16,s.toUShort);}
template<u32> u32 QStringToValue(QString &s,bool *ok) { CVT(u32,s.toUInt);}
template<u64> u64 QStringToValue(QString &s,bool *ok) { CVT(u64,s.toULongLong);}
template<i8 > i8  QStringToValue(QString &s,bool *ok) { CVT(i8 ,s.toShort);}
template<i16> i16 QStringToValue(QString &s,bool *ok) { CVT(i16,s.toShort);}
template<i32> i32 QStringToValue(QString &s,bool *ok) { CVT(i32,s.toInt);}
template<i64> i64 QStringToValue(QString &s,bool *ok) { CVT(i64,s.toLongLong);}
template<f32> f32 QStringToValue(QString &s,bool *ok) { CVT(f32,s.toFloat );}
template<f64> f64 QStringToValue(QString &s,bool *ok) { CVT(f64,s.toDouble);}
 

// Getters and Setters

void GetSetResonantTurn::Set_(device::Microscope *dc, f64 &v)
{ device::Microscope::Config c = dc->get_config();
  c.mutable_resonant_wrap()->set_turn_px(px);
  Guarded_Assert(_dc->set_config_nowait(c));
}
void f64& GetSetResonantTurn::Get_(device::Microscope *dc, f64 &v)
{ device::Microscope::Config c = dc->get_config(); 
	return c.resonant_wrap().turn_px();
}

void GetSetLines::Set_(device::Microscope *dc, i32 &v)
{ device::Microscope::Config c = dc->get_config();
  c.mutable_scanner3d()->mutable_scanner2d()->set_nscans(nlines/2);
  Guarded_Assert(_dc->set_config_nowait(c));
}
void i32& GetSetLines::Get_(device::Microscope *dc, i32 &v)
{ device::Microscope::Config c = dc->get_config(); 
	return 2*c.scanner3d().scanner2d().nscans();
}

void GetSetLSMVerticalRange::Set_(device::LinearScanMirror *dc, f64 &v)
{ _dc->setAmplitudeVolts(v);
}
void f64& GetSetLSMVerticalRange::Get_(device::LinearScanMirror *dc, f64 &v)
{ return dc_->getAmplitudeVolts();
}

void GetSetPockels::Set_(device::Pockels *dc, u32 &v)
{ dc_->setOpenVoltsNoWait(v/1000.0);
}
void u32& GetSetPockels::Get_(device::Pockels *dc, u32 &v)
{ return dc_->getOpenVolts();
}

}} //end fetch::ui
