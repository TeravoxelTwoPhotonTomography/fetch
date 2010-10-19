#include "microscope.pb.h"
#include "util/util-protobuf.h"
#include <iostream>
#include <string>
#include <google/protobuf/text_format.h>

using namespace fetch::cfg::device;
using namespace std;
using namespace google::protobuf;

int main(int argc,char* argv[])
{ 
  Microscope msg;
  set_unset_fields(&msg);
  string out;
  TextFormat::PrintToString(msg,&out);
  cout << out << endl;
  return 0;
}
