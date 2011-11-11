#include "google\protobuf\message.h"

//bool operator==(const google::protobuf::Message &a,const google::protobuf::Message &b);
//bool operator!=(const google::protobuf::Message &a,const google::protobuf::Message &b);
//bool operator==(const google::protobuf::Message *a,const google::protobuf::Message *b);
bool equals(const google::protobuf::Message *a,const google::protobuf::Message *b);
//bool operator!=(const google::protobuf::Message *a,const google::protobuf::Message *b);

namespace pb
{
  void lock();
  void unlock();
}
void set_unset_fields(google::protobuf::Message *msg);