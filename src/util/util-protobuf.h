#include "google\protobuf\message.h"

namespace pb
{
  void lock();
  void unlock();
}
void set_unset_fields(google::protobuf::Message *msg);