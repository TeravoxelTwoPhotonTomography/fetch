// Generated by the protocol buffer compiler.  DO NOT EDIT!

#define INTERNAL_SUPPRESS_PROTOBUF_FIELD_DEPRECATION
#include "linear_scan_mirror.pb.h"
#include <google/protobuf/stubs/once.h>
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/wire_format_lite_inl.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/reflection_ops.h>
#include <google/protobuf/wire_format.h>
// @@protoc_insertion_point(includes)

namespace fetch {
namespace cfg {
namespace device {

namespace {

const ::google::protobuf::Descriptor* LinearScanMirror_descriptor_ = NULL;
const ::google::protobuf::internal::GeneratedMessageReflection*
  LinearScanMirror_reflection_ = NULL;

}  // namespace


void protobuf_AssignDesc_linear_5fscan_5fmirror_2eproto() {
  protobuf_AddDesc_linear_5fscan_5fmirror_2eproto();
  const ::google::protobuf::FileDescriptor* file =
    ::google::protobuf::DescriptorPool::generated_pool()->FindFileByName(
      "linear_scan_mirror.proto");
  GOOGLE_CHECK(file != NULL);
  LinearScanMirror_descriptor_ = file->message_type(0);
  static const int LinearScanMirror_offsets_[4] = {
    GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(LinearScanMirror, v_lim_max_),
    GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(LinearScanMirror, v_lim_min_),
    GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(LinearScanMirror, vpp_),
    GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(LinearScanMirror, ao_channel_),
  };
  LinearScanMirror_reflection_ =
    new ::google::protobuf::internal::GeneratedMessageReflection(
      LinearScanMirror_descriptor_,
      LinearScanMirror::default_instance_,
      LinearScanMirror_offsets_,
      GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(LinearScanMirror, _has_bits_[0]),
      GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(LinearScanMirror, _unknown_fields_),
      -1,
      ::google::protobuf::DescriptorPool::generated_pool(),
      ::google::protobuf::MessageFactory::generated_factory(),
      sizeof(LinearScanMirror));
}

namespace {

GOOGLE_PROTOBUF_DECLARE_ONCE(protobuf_AssignDescriptors_once_);
inline void protobuf_AssignDescriptorsOnce() {
  ::google::protobuf::GoogleOnceInit(&protobuf_AssignDescriptors_once_,
                 &protobuf_AssignDesc_linear_5fscan_5fmirror_2eproto);
}

void protobuf_RegisterTypes(const ::std::string&) {
  protobuf_AssignDescriptorsOnce();
  ::google::protobuf::MessageFactory::InternalRegisterGeneratedMessage(
    LinearScanMirror_descriptor_, &LinearScanMirror::default_instance());
}

}  // namespace

void protobuf_ShutdownFile_linear_5fscan_5fmirror_2eproto() {
  delete LinearScanMirror::default_instance_;
  delete LinearScanMirror_reflection_;
}

void protobuf_AddDesc_linear_5fscan_5fmirror_2eproto() {
  static bool already_here = false;
  if (already_here) return;
  already_here = true;
  GOOGLE_PROTOBUF_VERIFY_VERSION;

  ::google::protobuf::DescriptorPool::InternalAddGeneratedFile(
    "\n\030linear_scan_mirror.proto\022\020fetch.cfg.de"
    "vice\"q\n\020LinearScanMirror\022\025\n\tv_lim_max\030\001 "
    "\001(\001:\00210\022\026\n\tv_lim_min\030\002 \001(\001:\003-10\022\017\n\003vpp\030\003"
    " \001(\001:\00210\022\035\n\nao_channel\030\004 \001(\t:\t/Dev1/ao0", 159);
  ::google::protobuf::MessageFactory::InternalRegisterGeneratedFile(
    "linear_scan_mirror.proto", &protobuf_RegisterTypes);
  LinearScanMirror::default_instance_ = new LinearScanMirror();
  LinearScanMirror::default_instance_->InitAsDefaultInstance();
  ::google::protobuf::internal::OnShutdown(&protobuf_ShutdownFile_linear_5fscan_5fmirror_2eproto);
}

// Force AddDescriptors() to be called at static initialization time.
struct StaticDescriptorInitializer_linear_5fscan_5fmirror_2eproto {
  StaticDescriptorInitializer_linear_5fscan_5fmirror_2eproto() {
    protobuf_AddDesc_linear_5fscan_5fmirror_2eproto();
  }
} static_descriptor_initializer_linear_5fscan_5fmirror_2eproto_;


// ===================================================================

const ::std::string LinearScanMirror::_default_ao_channel_("/Dev1/ao0");
#ifndef _MSC_VER
const int LinearScanMirror::kVLimMaxFieldNumber;
const int LinearScanMirror::kVLimMinFieldNumber;
const int LinearScanMirror::kVppFieldNumber;
const int LinearScanMirror::kAoChannelFieldNumber;
#endif  // !_MSC_VER

LinearScanMirror::LinearScanMirror()
  : ::google::protobuf::Message() {
  SharedCtor();
}

void LinearScanMirror::InitAsDefaultInstance() {
}

LinearScanMirror::LinearScanMirror(const LinearScanMirror& from)
  : ::google::protobuf::Message() {
  SharedCtor();
  MergeFrom(from);
}

void LinearScanMirror::SharedCtor() {
  _cached_size_ = 0;
  v_lim_max_ = 10;
  v_lim_min_ = -10;
  vpp_ = 10;
  ao_channel_ = const_cast< ::std::string*>(&_default_ao_channel_);
  ::memset(_has_bits_, 0, sizeof(_has_bits_));
}

LinearScanMirror::~LinearScanMirror() {
  SharedDtor();
}

void LinearScanMirror::SharedDtor() {
  if (ao_channel_ != &_default_ao_channel_) {
    delete ao_channel_;
  }
  if (this != default_instance_) {
  }
}

void LinearScanMirror::SetCachedSize(int size) const {
  GOOGLE_SAFE_CONCURRENT_WRITES_BEGIN();
  _cached_size_ = size;
  GOOGLE_SAFE_CONCURRENT_WRITES_END();
}
const ::google::protobuf::Descriptor* LinearScanMirror::descriptor() {
  protobuf_AssignDescriptorsOnce();
  return LinearScanMirror_descriptor_;
}

const LinearScanMirror& LinearScanMirror::default_instance() {
  if (default_instance_ == NULL) protobuf_AddDesc_linear_5fscan_5fmirror_2eproto();  return *default_instance_;
}

LinearScanMirror* LinearScanMirror::default_instance_ = NULL;

LinearScanMirror* LinearScanMirror::New() const {
  return new LinearScanMirror;
}

void LinearScanMirror::Clear() {
  if (_has_bits_[0 / 32] & (0xffu << (0 % 32))) {
    v_lim_max_ = 10;
    v_lim_min_ = -10;
    vpp_ = 10;
    if (_has_bit(3)) {
      if (ao_channel_ != &_default_ao_channel_) {
        ao_channel_->assign(_default_ao_channel_);
      }
    }
  }
  ::memset(_has_bits_, 0, sizeof(_has_bits_));
  mutable_unknown_fields()->Clear();
}

bool LinearScanMirror::MergePartialFromCodedStream(
    ::google::protobuf::io::CodedInputStream* input) {
#define DO_(EXPRESSION) if (!(EXPRESSION)) return false
  ::google::protobuf::uint32 tag;
  while ((tag = input->ReadTag()) != 0) {
    switch (::google::protobuf::internal::WireFormatLite::GetTagFieldNumber(tag)) {
      // optional double v_lim_max = 1 [default = 10];
      case 1: {
        if (::google::protobuf::internal::WireFormatLite::GetTagWireType(tag) ==
            ::google::protobuf::internal::WireFormatLite::WIRETYPE_FIXED64) {
          DO_((::google::protobuf::internal::WireFormatLite::ReadPrimitive<
                   double, ::google::protobuf::internal::WireFormatLite::TYPE_DOUBLE>(
                 input, &v_lim_max_)));
          _set_bit(0);
        } else {
          goto handle_uninterpreted;
        }
        if (input->ExpectTag(17)) goto parse_v_lim_min;
        break;
      }
      
      // optional double v_lim_min = 2 [default = -10];
      case 2: {
        if (::google::protobuf::internal::WireFormatLite::GetTagWireType(tag) ==
            ::google::protobuf::internal::WireFormatLite::WIRETYPE_FIXED64) {
         parse_v_lim_min:
          DO_((::google::protobuf::internal::WireFormatLite::ReadPrimitive<
                   double, ::google::protobuf::internal::WireFormatLite::TYPE_DOUBLE>(
                 input, &v_lim_min_)));
          _set_bit(1);
        } else {
          goto handle_uninterpreted;
        }
        if (input->ExpectTag(25)) goto parse_vpp;
        break;
      }
      
      // optional double vpp = 3 [default = 10];
      case 3: {
        if (::google::protobuf::internal::WireFormatLite::GetTagWireType(tag) ==
            ::google::protobuf::internal::WireFormatLite::WIRETYPE_FIXED64) {
         parse_vpp:
          DO_((::google::protobuf::internal::WireFormatLite::ReadPrimitive<
                   double, ::google::protobuf::internal::WireFormatLite::TYPE_DOUBLE>(
                 input, &vpp_)));
          _set_bit(2);
        } else {
          goto handle_uninterpreted;
        }
        if (input->ExpectTag(34)) goto parse_ao_channel;
        break;
      }
      
      // optional string ao_channel = 4 [default = "/Dev1/ao0"];
      case 4: {
        if (::google::protobuf::internal::WireFormatLite::GetTagWireType(tag) ==
            ::google::protobuf::internal::WireFormatLite::WIRETYPE_LENGTH_DELIMITED) {
         parse_ao_channel:
          DO_(::google::protobuf::internal::WireFormatLite::ReadString(
                input, this->mutable_ao_channel()));
          ::google::protobuf::internal::WireFormat::VerifyUTF8String(
            this->ao_channel().data(), this->ao_channel().length(),
            ::google::protobuf::internal::WireFormat::PARSE);
        } else {
          goto handle_uninterpreted;
        }
        if (input->ExpectAtEnd()) return true;
        break;
      }
      
      default: {
      handle_uninterpreted:
        if (::google::protobuf::internal::WireFormatLite::GetTagWireType(tag) ==
            ::google::protobuf::internal::WireFormatLite::WIRETYPE_END_GROUP) {
          return true;
        }
        DO_(::google::protobuf::internal::WireFormat::SkipField(
              input, tag, mutable_unknown_fields()));
        break;
      }
    }
  }
  return true;
#undef DO_
}

void LinearScanMirror::SerializeWithCachedSizes(
    ::google::protobuf::io::CodedOutputStream* output) const {
  // optional double v_lim_max = 1 [default = 10];
  if (_has_bit(0)) {
    ::google::protobuf::internal::WireFormatLite::WriteDouble(1, this->v_lim_max(), output);
  }
  
  // optional double v_lim_min = 2 [default = -10];
  if (_has_bit(1)) {
    ::google::protobuf::internal::WireFormatLite::WriteDouble(2, this->v_lim_min(), output);
  }
  
  // optional double vpp = 3 [default = 10];
  if (_has_bit(2)) {
    ::google::protobuf::internal::WireFormatLite::WriteDouble(3, this->vpp(), output);
  }
  
  // optional string ao_channel = 4 [default = "/Dev1/ao0"];
  if (_has_bit(3)) {
    ::google::protobuf::internal::WireFormat::VerifyUTF8String(
      this->ao_channel().data(), this->ao_channel().length(),
      ::google::protobuf::internal::WireFormat::SERIALIZE);
    ::google::protobuf::internal::WireFormatLite::WriteString(
      4, this->ao_channel(), output);
  }
  
  if (!unknown_fields().empty()) {
    ::google::protobuf::internal::WireFormat::SerializeUnknownFields(
        unknown_fields(), output);
  }
}

::google::protobuf::uint8* LinearScanMirror::SerializeWithCachedSizesToArray(
    ::google::protobuf::uint8* target) const {
  // optional double v_lim_max = 1 [default = 10];
  if (_has_bit(0)) {
    target = ::google::protobuf::internal::WireFormatLite::WriteDoubleToArray(1, this->v_lim_max(), target);
  }
  
  // optional double v_lim_min = 2 [default = -10];
  if (_has_bit(1)) {
    target = ::google::protobuf::internal::WireFormatLite::WriteDoubleToArray(2, this->v_lim_min(), target);
  }
  
  // optional double vpp = 3 [default = 10];
  if (_has_bit(2)) {
    target = ::google::protobuf::internal::WireFormatLite::WriteDoubleToArray(3, this->vpp(), target);
  }
  
  // optional string ao_channel = 4 [default = "/Dev1/ao0"];
  if (_has_bit(3)) {
    ::google::protobuf::internal::WireFormat::VerifyUTF8String(
      this->ao_channel().data(), this->ao_channel().length(),
      ::google::protobuf::internal::WireFormat::SERIALIZE);
    target =
      ::google::protobuf::internal::WireFormatLite::WriteStringToArray(
        4, this->ao_channel(), target);
  }
  
  if (!unknown_fields().empty()) {
    target = ::google::protobuf::internal::WireFormat::SerializeUnknownFieldsToArray(
        unknown_fields(), target);
  }
  return target;
}

int LinearScanMirror::ByteSize() const {
  int total_size = 0;
  
  if (_has_bits_[0 / 32] & (0xffu << (0 % 32))) {
    // optional double v_lim_max = 1 [default = 10];
    if (has_v_lim_max()) {
      total_size += 1 + 8;
    }
    
    // optional double v_lim_min = 2 [default = -10];
    if (has_v_lim_min()) {
      total_size += 1 + 8;
    }
    
    // optional double vpp = 3 [default = 10];
    if (has_vpp()) {
      total_size += 1 + 8;
    }
    
    // optional string ao_channel = 4 [default = "/Dev1/ao0"];
    if (has_ao_channel()) {
      total_size += 1 +
        ::google::protobuf::internal::WireFormatLite::StringSize(
          this->ao_channel());
    }
    
  }
  if (!unknown_fields().empty()) {
    total_size +=
      ::google::protobuf::internal::WireFormat::ComputeUnknownFieldsSize(
        unknown_fields());
  }
  GOOGLE_SAFE_CONCURRENT_WRITES_BEGIN();
  _cached_size_ = total_size;
  GOOGLE_SAFE_CONCURRENT_WRITES_END();
  return total_size;
}

void LinearScanMirror::MergeFrom(const ::google::protobuf::Message& from) {
  GOOGLE_CHECK_NE(&from, this);
  const LinearScanMirror* source =
    ::google::protobuf::internal::dynamic_cast_if_available<const LinearScanMirror*>(
      &from);
  if (source == NULL) {
    ::google::protobuf::internal::ReflectionOps::Merge(from, this);
  } else {
    MergeFrom(*source);
  }
}

void LinearScanMirror::MergeFrom(const LinearScanMirror& from) {
  GOOGLE_CHECK_NE(&from, this);
  if (from._has_bits_[0 / 32] & (0xffu << (0 % 32))) {
    if (from._has_bit(0)) {
      set_v_lim_max(from.v_lim_max());
    }
    if (from._has_bit(1)) {
      set_v_lim_min(from.v_lim_min());
    }
    if (from._has_bit(2)) {
      set_vpp(from.vpp());
    }
    if (from._has_bit(3)) {
      set_ao_channel(from.ao_channel());
    }
  }
  mutable_unknown_fields()->MergeFrom(from.unknown_fields());
}

void LinearScanMirror::CopyFrom(const ::google::protobuf::Message& from) {
  if (&from == this) return;
  Clear();
  MergeFrom(from);
}

void LinearScanMirror::CopyFrom(const LinearScanMirror& from) {
  if (&from == this) return;
  Clear();
  MergeFrom(from);
}

bool LinearScanMirror::IsInitialized() const {
  
  return true;
}

void LinearScanMirror::Swap(LinearScanMirror* other) {
  if (other != this) {
    std::swap(v_lim_max_, other->v_lim_max_);
    std::swap(v_lim_min_, other->v_lim_min_);
    std::swap(vpp_, other->vpp_);
    std::swap(ao_channel_, other->ao_channel_);
    std::swap(_has_bits_[0], other->_has_bits_[0]);
    _unknown_fields_.Swap(&other->_unknown_fields_);
    std::swap(_cached_size_, other->_cached_size_);
  }
}

::google::protobuf::Metadata LinearScanMirror::GetMetadata() const {
  protobuf_AssignDescriptorsOnce();
  ::google::protobuf::Metadata metadata;
  metadata.descriptor = LinearScanMirror_descriptor_;
  metadata.reflection = LinearScanMirror_reflection_;
  return metadata;
}


// @@protoc_insertion_point(namespace_scope)

}  // namespace device
}  // namespace cfg
}  // namespace fetch

// @@protoc_insertion_point(global_scope)