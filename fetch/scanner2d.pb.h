// Generated by the protocol buffer compiler.  DO NOT EDIT!
// source: scanner2d.proto

#ifndef PROTOBUF_scanner2d_2eproto__INCLUDED
#define PROTOBUF_scanner2d_2eproto__INCLUDED

#include <string>

#include <google/protobuf/stubs/common.h>

#if GOOGLE_PROTOBUF_VERSION < 2003000
#error This file was generated by a newer version of protoc which is
#error incompatible with your Protocol Buffer headers.  Please update
#error your headers.
#endif
#if 2003000 < GOOGLE_PROTOBUF_MIN_PROTOC_VERSION
#error This file was generated by an older version of protoc which is
#error incompatible with your Protocol Buffer headers.  Please
#error regenerate this file with a newer version of protoc.
#endif

#include <google/protobuf/generated_message_util.h>
#include <google/protobuf/repeated_field.h>
#include <google/protobuf/extension_set.h>
#include <google/protobuf/generated_message_reflection.h>
#include "digitizer.pb.h"
#include "pockels.pb.h"
#include "shutter.pb.h"
#include "linear_scan_mirror.pb.h"
// @@protoc_insertion_point(includes)

namespace fetch {
namespace cfg {
namespace device {

// Internal implementation detail -- do not call these.
void  protobuf_AddDesc_scanner2d_2eproto();
void protobuf_AssignDesc_scanner2d_2eproto();
void protobuf_ShutdownFile_scanner2d_2eproto();

class Scanner2D;

// ===================================================================

class Scanner2D : public ::google::protobuf::Message {
 public:
  Scanner2D();
  virtual ~Scanner2D();
  
  Scanner2D(const Scanner2D& from);
  
  inline Scanner2D& operator=(const Scanner2D& from) {
    CopyFrom(from);
    return *this;
  }
  
  inline const ::google::protobuf::UnknownFieldSet& unknown_fields() const {
    return _unknown_fields_;
  }
  
  inline ::google::protobuf::UnknownFieldSet* mutable_unknown_fields() {
    return &_unknown_fields_;
  }
  
  static const ::google::protobuf::Descriptor* descriptor();
  static const Scanner2D& default_instance();
  
  void Swap(Scanner2D* other);
  
  // implements Message ----------------------------------------------
  
  Scanner2D* New() const;
  void CopyFrom(const ::google::protobuf::Message& from);
  void MergeFrom(const ::google::protobuf::Message& from);
  void CopyFrom(const Scanner2D& from);
  void MergeFrom(const Scanner2D& from);
  void Clear();
  bool IsInitialized() const;
  
  int ByteSize() const;
  bool MergePartialFromCodedStream(
      ::google::protobuf::io::CodedInputStream* input);
  void SerializeWithCachedSizes(
      ::google::protobuf::io::CodedOutputStream* output) const;
  ::google::protobuf::uint8* SerializeWithCachedSizesToArray(::google::protobuf::uint8* output) const;
  int GetCachedSize() const { return _cached_size_; }
  private:
  void SharedCtor();
  void SharedDtor();
  void SetCachedSize(int size) const;
  public:
  
  ::google::protobuf::Metadata GetMetadata() const;
  
  // nested types ----------------------------------------------------
  
  // accessors -------------------------------------------------------
  
  // optional double frequency_hz = 1 [default = 7920];
  inline bool has_frequency_hz() const;
  inline void clear_frequency_hz();
  static const int kFrequencyHzFieldNumber = 1;
  inline double frequency_hz() const;
  inline void set_frequency_hz(double value);
  
  // optional uint32 nscans = 2 [default = 512];
  inline bool has_nscans() const;
  inline void clear_nscans();
  static const int kNscansFieldNumber = 2;
  inline ::google::protobuf::uint32 nscans() const;
  inline void set_nscans(::google::protobuf::uint32 value);
  
  // optional float line_duty_cycle = 3 [default = 0.95];
  inline bool has_line_duty_cycle() const;
  inline void clear_line_duty_cycle();
  static const int kLineDutyCycleFieldNumber = 3;
  inline float line_duty_cycle() const;
  inline void set_line_duty_cycle(float value);
  
  // optional uint32 line_trigger_src = 4 [default = 1];
  inline bool has_line_trigger_src() const;
  inline void clear_line_trigger_src();
  static const int kLineTriggerSrcFieldNumber = 4;
  inline ::google::protobuf::uint32 line_trigger_src() const;
  inline void set_line_trigger_src(::google::protobuf::uint32 value);
  
  // optional string trigger = 5 [default = "APFI0"];
  inline bool has_trigger() const;
  inline void clear_trigger();
  static const int kTriggerFieldNumber = 5;
  inline const ::std::string& trigger() const;
  inline void set_trigger(const ::std::string& value);
  inline void set_trigger(const char* value);
  inline void set_trigger(const char* value, size_t size);
  inline ::std::string* mutable_trigger();
  
  // optional string armstart = 6 [default = "RTSI2"];
  inline bool has_armstart() const;
  inline void clear_armstart();
  static const int kArmstartFieldNumber = 6;
  inline const ::std::string& armstart() const;
  inline void set_armstart(const ::std::string& value);
  inline void set_armstart(const char* value);
  inline void set_armstart(const char* value, size_t size);
  inline ::std::string* mutable_armstart();
  
  // optional string clock = 7 [default = "Ctr1InternalOutput"];
  inline bool has_clock() const;
  inline void clear_clock();
  static const int kClockFieldNumber = 7;
  inline const ::std::string& clock() const;
  inline void set_clock(const ::std::string& value);
  inline void set_clock(const char* value);
  inline void set_clock(const char* value, size_t size);
  inline ::std::string* mutable_clock();
  
  // optional string ctr = 8 [default = "/Dev1/ctr1"];
  inline bool has_ctr() const;
  inline void clear_ctr();
  static const int kCtrFieldNumber = 8;
  inline const ::std::string& ctr() const;
  inline void set_ctr(const ::std::string& value);
  inline void set_ctr(const char* value);
  inline void set_ctr(const char* value, size_t size);
  inline ::std::string* mutable_ctr();
  
  // optional string ctr_alt = 9 [default = "/Dev1/ctr0"];
  inline bool has_ctr_alt() const;
  inline void clear_ctr_alt();
  static const int kCtrAltFieldNumber = 9;
  inline const ::std::string& ctr_alt() const;
  inline void set_ctr_alt(const ::std::string& value);
  inline void set_ctr_alt(const char* value);
  inline void set_ctr_alt(const char* value, size_t size);
  inline ::std::string* mutable_ctr_alt();
  
  // optional uint32 ao_samples_per_frame = 10 [default = 16384];
  inline bool has_ao_samples_per_frame() const;
  inline void clear_ao_samples_per_frame();
  static const int kAoSamplesPerFrameFieldNumber = 10;
  inline ::google::protobuf::uint32 ao_samples_per_frame() const;
  inline void set_ao_samples_per_frame(::google::protobuf::uint32 value);
  
  // required .fetch.cfg.device.Digitizer digitizer = 11;
  inline bool has_digitizer() const;
  inline void clear_digitizer();
  static const int kDigitizerFieldNumber = 11;
  inline const ::fetch::cfg::device::Digitizer& digitizer() const;
  inline ::fetch::cfg::device::Digitizer* mutable_digitizer();
  
  // required .fetch.cfg.device.Pockels pockels = 12;
  inline bool has_pockels() const;
  inline void clear_pockels();
  static const int kPockelsFieldNumber = 12;
  inline const ::fetch::cfg::device::Pockels& pockels() const;
  inline ::fetch::cfg::device::Pockels* mutable_pockels();
  
  // required .fetch.cfg.device.Shutter shutter = 13;
  inline bool has_shutter() const;
  inline void clear_shutter();
  static const int kShutterFieldNumber = 13;
  inline const ::fetch::cfg::device::Shutter& shutter() const;
  inline ::fetch::cfg::device::Shutter* mutable_shutter();
  
  // required .fetch.cfg.device.LinearScanMirror linear_scan_mirror = 14;
  inline bool has_linear_scan_mirror() const;
  inline void clear_linear_scan_mirror();
  static const int kLinearScanMirrorFieldNumber = 14;
  inline const ::fetch::cfg::device::LinearScanMirror& linear_scan_mirror() const;
  inline ::fetch::cfg::device::LinearScanMirror* mutable_linear_scan_mirror();
  
  // @@protoc_insertion_point(class_scope:fetch.cfg.device.Scanner2D)
 private:
  ::google::protobuf::UnknownFieldSet _unknown_fields_;
  mutable int _cached_size_;
  
  double frequency_hz_;
  ::google::protobuf::uint32 nscans_;
  float line_duty_cycle_;
  ::google::protobuf::uint32 line_trigger_src_;
  ::std::string* trigger_;
  static const ::std::string _default_trigger_;
  ::std::string* armstart_;
  static const ::std::string _default_armstart_;
  ::std::string* clock_;
  static const ::std::string _default_clock_;
  ::std::string* ctr_;
  static const ::std::string _default_ctr_;
  ::std::string* ctr_alt_;
  static const ::std::string _default_ctr_alt_;
  ::google::protobuf::uint32 ao_samples_per_frame_;
  ::fetch::cfg::device::Digitizer* digitizer_;
  ::fetch::cfg::device::Pockels* pockels_;
  ::fetch::cfg::device::Shutter* shutter_;
  ::fetch::cfg::device::LinearScanMirror* linear_scan_mirror_;
  friend void  protobuf_AddDesc_scanner2d_2eproto();
  friend void protobuf_AssignDesc_scanner2d_2eproto();
  friend void protobuf_ShutdownFile_scanner2d_2eproto();
  
  ::google::protobuf::uint32 _has_bits_[(14 + 31) / 32];
  
  // WHY DOES & HAVE LOWER PRECEDENCE THAN != !?
  inline bool _has_bit(int index) const {
    return (_has_bits_[index / 32] & (1u << (index % 32))) != 0;
  }
  inline void _set_bit(int index) {
    _has_bits_[index / 32] |= (1u << (index % 32));
  }
  inline void _clear_bit(int index) {
    _has_bits_[index / 32] &= ~(1u << (index % 32));
  }
  
  void InitAsDefaultInstance();
  static Scanner2D* default_instance_;
};
// ===================================================================


// ===================================================================

// Scanner2D

// optional double frequency_hz = 1 [default = 7920];
inline bool Scanner2D::has_frequency_hz() const {
  return _has_bit(0);
}
inline void Scanner2D::clear_frequency_hz() {
  frequency_hz_ = 7920;
  _clear_bit(0);
}
inline double Scanner2D::frequency_hz() const {
  return frequency_hz_;
}
inline void Scanner2D::set_frequency_hz(double value) {
  _set_bit(0);
  frequency_hz_ = value;
}

// optional uint32 nscans = 2 [default = 512];
inline bool Scanner2D::has_nscans() const {
  return _has_bit(1);
}
inline void Scanner2D::clear_nscans() {
  nscans_ = 512u;
  _clear_bit(1);
}
inline ::google::protobuf::uint32 Scanner2D::nscans() const {
  return nscans_;
}
inline void Scanner2D::set_nscans(::google::protobuf::uint32 value) {
  _set_bit(1);
  nscans_ = value;
}

// optional float line_duty_cycle = 3 [default = 0.95];
inline bool Scanner2D::has_line_duty_cycle() const {
  return _has_bit(2);
}
inline void Scanner2D::clear_line_duty_cycle() {
  line_duty_cycle_ = 0.95f;
  _clear_bit(2);
}
inline float Scanner2D::line_duty_cycle() const {
  return line_duty_cycle_;
}
inline void Scanner2D::set_line_duty_cycle(float value) {
  _set_bit(2);
  line_duty_cycle_ = value;
}

// optional uint32 line_trigger_src = 4 [default = 1];
inline bool Scanner2D::has_line_trigger_src() const {
  return _has_bit(3);
}
inline void Scanner2D::clear_line_trigger_src() {
  line_trigger_src_ = 1u;
  _clear_bit(3);
}
inline ::google::protobuf::uint32 Scanner2D::line_trigger_src() const {
  return line_trigger_src_;
}
inline void Scanner2D::set_line_trigger_src(::google::protobuf::uint32 value) {
  _set_bit(3);
  line_trigger_src_ = value;
}

// optional string trigger = 5 [default = "APFI0"];
inline bool Scanner2D::has_trigger() const {
  return _has_bit(4);
}
inline void Scanner2D::clear_trigger() {
  if (trigger_ != &_default_trigger_) {
    trigger_->assign(_default_trigger_);
  }
  _clear_bit(4);
}
inline const ::std::string& Scanner2D::trigger() const {
  return *trigger_;
}
inline void Scanner2D::set_trigger(const ::std::string& value) {
  _set_bit(4);
  if (trigger_ == &_default_trigger_) {
    trigger_ = new ::std::string;
  }
  trigger_->assign(value);
}
inline void Scanner2D::set_trigger(const char* value) {
  _set_bit(4);
  if (trigger_ == &_default_trigger_) {
    trigger_ = new ::std::string;
  }
  trigger_->assign(value);
}
inline void Scanner2D::set_trigger(const char* value, size_t size) {
  _set_bit(4);
  if (trigger_ == &_default_trigger_) {
    trigger_ = new ::std::string;
  }
  trigger_->assign(reinterpret_cast<const char*>(value), size);
}
inline ::std::string* Scanner2D::mutable_trigger() {
  _set_bit(4);
  if (trigger_ == &_default_trigger_) {
    trigger_ = new ::std::string(_default_trigger_);
  }
  return trigger_;
}

// optional string armstart = 6 [default = "RTSI2"];
inline bool Scanner2D::has_armstart() const {
  return _has_bit(5);
}
inline void Scanner2D::clear_armstart() {
  if (armstart_ != &_default_armstart_) {
    armstart_->assign(_default_armstart_);
  }
  _clear_bit(5);
}
inline const ::std::string& Scanner2D::armstart() const {
  return *armstart_;
}
inline void Scanner2D::set_armstart(const ::std::string& value) {
  _set_bit(5);
  if (armstart_ == &_default_armstart_) {
    armstart_ = new ::std::string;
  }
  armstart_->assign(value);
}
inline void Scanner2D::set_armstart(const char* value) {
  _set_bit(5);
  if (armstart_ == &_default_armstart_) {
    armstart_ = new ::std::string;
  }
  armstart_->assign(value);
}
inline void Scanner2D::set_armstart(const char* value, size_t size) {
  _set_bit(5);
  if (armstart_ == &_default_armstart_) {
    armstart_ = new ::std::string;
  }
  armstart_->assign(reinterpret_cast<const char*>(value), size);
}
inline ::std::string* Scanner2D::mutable_armstart() {
  _set_bit(5);
  if (armstart_ == &_default_armstart_) {
    armstart_ = new ::std::string(_default_armstart_);
  }
  return armstart_;
}

// optional string clock = 7 [default = "Ctr1InternalOutput"];
inline bool Scanner2D::has_clock() const {
  return _has_bit(6);
}
inline void Scanner2D::clear_clock() {
  if (clock_ != &_default_clock_) {
    clock_->assign(_default_clock_);
  }
  _clear_bit(6);
}
inline const ::std::string& Scanner2D::clock() const {
  return *clock_;
}
inline void Scanner2D::set_clock(const ::std::string& value) {
  _set_bit(6);
  if (clock_ == &_default_clock_) {
    clock_ = new ::std::string;
  }
  clock_->assign(value);
}
inline void Scanner2D::set_clock(const char* value) {
  _set_bit(6);
  if (clock_ == &_default_clock_) {
    clock_ = new ::std::string;
  }
  clock_->assign(value);
}
inline void Scanner2D::set_clock(const char* value, size_t size) {
  _set_bit(6);
  if (clock_ == &_default_clock_) {
    clock_ = new ::std::string;
  }
  clock_->assign(reinterpret_cast<const char*>(value), size);
}
inline ::std::string* Scanner2D::mutable_clock() {
  _set_bit(6);
  if (clock_ == &_default_clock_) {
    clock_ = new ::std::string(_default_clock_);
  }
  return clock_;
}

// optional string ctr = 8 [default = "/Dev1/ctr1"];
inline bool Scanner2D::has_ctr() const {
  return _has_bit(7);
}
inline void Scanner2D::clear_ctr() {
  if (ctr_ != &_default_ctr_) {
    ctr_->assign(_default_ctr_);
  }
  _clear_bit(7);
}
inline const ::std::string& Scanner2D::ctr() const {
  return *ctr_;
}
inline void Scanner2D::set_ctr(const ::std::string& value) {
  _set_bit(7);
  if (ctr_ == &_default_ctr_) {
    ctr_ = new ::std::string;
  }
  ctr_->assign(value);
}
inline void Scanner2D::set_ctr(const char* value) {
  _set_bit(7);
  if (ctr_ == &_default_ctr_) {
    ctr_ = new ::std::string;
  }
  ctr_->assign(value);
}
inline void Scanner2D::set_ctr(const char* value, size_t size) {
  _set_bit(7);
  if (ctr_ == &_default_ctr_) {
    ctr_ = new ::std::string;
  }
  ctr_->assign(reinterpret_cast<const char*>(value), size);
}
inline ::std::string* Scanner2D::mutable_ctr() {
  _set_bit(7);
  if (ctr_ == &_default_ctr_) {
    ctr_ = new ::std::string(_default_ctr_);
  }
  return ctr_;
}

// optional string ctr_alt = 9 [default = "/Dev1/ctr0"];
inline bool Scanner2D::has_ctr_alt() const {
  return _has_bit(8);
}
inline void Scanner2D::clear_ctr_alt() {
  if (ctr_alt_ != &_default_ctr_alt_) {
    ctr_alt_->assign(_default_ctr_alt_);
  }
  _clear_bit(8);
}
inline const ::std::string& Scanner2D::ctr_alt() const {
  return *ctr_alt_;
}
inline void Scanner2D::set_ctr_alt(const ::std::string& value) {
  _set_bit(8);
  if (ctr_alt_ == &_default_ctr_alt_) {
    ctr_alt_ = new ::std::string;
  }
  ctr_alt_->assign(value);
}
inline void Scanner2D::set_ctr_alt(const char* value) {
  _set_bit(8);
  if (ctr_alt_ == &_default_ctr_alt_) {
    ctr_alt_ = new ::std::string;
  }
  ctr_alt_->assign(value);
}
inline void Scanner2D::set_ctr_alt(const char* value, size_t size) {
  _set_bit(8);
  if (ctr_alt_ == &_default_ctr_alt_) {
    ctr_alt_ = new ::std::string;
  }
  ctr_alt_->assign(reinterpret_cast<const char*>(value), size);
}
inline ::std::string* Scanner2D::mutable_ctr_alt() {
  _set_bit(8);
  if (ctr_alt_ == &_default_ctr_alt_) {
    ctr_alt_ = new ::std::string(_default_ctr_alt_);
  }
  return ctr_alt_;
}

// optional uint32 ao_samples_per_frame = 10 [default = 16384];
inline bool Scanner2D::has_ao_samples_per_frame() const {
  return _has_bit(9);
}
inline void Scanner2D::clear_ao_samples_per_frame() {
  ao_samples_per_frame_ = 16384u;
  _clear_bit(9);
}
inline ::google::protobuf::uint32 Scanner2D::ao_samples_per_frame() const {
  return ao_samples_per_frame_;
}
inline void Scanner2D::set_ao_samples_per_frame(::google::protobuf::uint32 value) {
  _set_bit(9);
  ao_samples_per_frame_ = value;
}

// required .fetch.cfg.device.Digitizer digitizer = 11;
inline bool Scanner2D::has_digitizer() const {
  return _has_bit(10);
}
inline void Scanner2D::clear_digitizer() {
  if (digitizer_ != NULL) digitizer_->::fetch::cfg::device::Digitizer::Clear();
  _clear_bit(10);
}
inline const ::fetch::cfg::device::Digitizer& Scanner2D::digitizer() const {
  return digitizer_ != NULL ? *digitizer_ : *default_instance_->digitizer_;
}
inline ::fetch::cfg::device::Digitizer* Scanner2D::mutable_digitizer() {
  _set_bit(10);
  if (digitizer_ == NULL) digitizer_ = new ::fetch::cfg::device::Digitizer;
  return digitizer_;
}

// required .fetch.cfg.device.Pockels pockels = 12;
inline bool Scanner2D::has_pockels() const {
  return _has_bit(11);
}
inline void Scanner2D::clear_pockels() {
  if (pockels_ != NULL) pockels_->::fetch::cfg::device::Pockels::Clear();
  _clear_bit(11);
}
inline const ::fetch::cfg::device::Pockels& Scanner2D::pockels() const {
  return pockels_ != NULL ? *pockels_ : *default_instance_->pockels_;
}
inline ::fetch::cfg::device::Pockels* Scanner2D::mutable_pockels() {
  _set_bit(11);
  if (pockels_ == NULL) pockels_ = new ::fetch::cfg::device::Pockels;
  return pockels_;
}

// required .fetch.cfg.device.Shutter shutter = 13;
inline bool Scanner2D::has_shutter() const {
  return _has_bit(12);
}
inline void Scanner2D::clear_shutter() {
  if (shutter_ != NULL) shutter_->::fetch::cfg::device::Shutter::Clear();
  _clear_bit(12);
}
inline const ::fetch::cfg::device::Shutter& Scanner2D::shutter() const {
  return shutter_ != NULL ? *shutter_ : *default_instance_->shutter_;
}
inline ::fetch::cfg::device::Shutter* Scanner2D::mutable_shutter() {
  _set_bit(12);
  if (shutter_ == NULL) shutter_ = new ::fetch::cfg::device::Shutter;
  return shutter_;
}

// required .fetch.cfg.device.LinearScanMirror linear_scan_mirror = 14;
inline bool Scanner2D::has_linear_scan_mirror() const {
  return _has_bit(13);
}
inline void Scanner2D::clear_linear_scan_mirror() {
  if (linear_scan_mirror_ != NULL) linear_scan_mirror_->::fetch::cfg::device::LinearScanMirror::Clear();
  _clear_bit(13);
}
inline const ::fetch::cfg::device::LinearScanMirror& Scanner2D::linear_scan_mirror() const {
  return linear_scan_mirror_ != NULL ? *linear_scan_mirror_ : *default_instance_->linear_scan_mirror_;
}
inline ::fetch::cfg::device::LinearScanMirror* Scanner2D::mutable_linear_scan_mirror() {
  _set_bit(13);
  if (linear_scan_mirror_ == NULL) linear_scan_mirror_ = new ::fetch::cfg::device::LinearScanMirror;
  return linear_scan_mirror_;
}


// @@protoc_insertion_point(namespace_scope)

}  // namespace device
}  // namespace cfg
}  // namespace fetch

#ifndef SWIG
namespace google {
namespace protobuf {


}  // namespace google
}  // namespace protobuf
#endif  // SWIG

// @@protoc_insertion_point(global_scope)

#endif  // PROTOBUF_scanner2d_2eproto__INCLUDED
