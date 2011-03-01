#include "microscope.pb.h"
#include "util/util-protobuf.h"
#include <iostream>
#include <string>
#include <google/protobuf/text_format.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <io.h>
#include <fcntl.h>

using namespace fetch::cfg::device;
using namespace std;
using namespace google::protobuf;
using namespace google::protobuf::io;

int main(int argc,char* argv[])
{ 
  Microscope msg;  
  if(argc>1)
  { int fid = _open(argv[1],_O_BINARY|_O_RDONLY|_O_SEQUENTIAL);
    if(fid==-1) goto ErrorCouldNotOpenFile;

    FileInputStream in(fid);
    msg.ParseFromZeroCopyStream(&in);
  }
  //set_unset_fields(&msg);
  
  { string out;
    TextFormat::PrintToString(msg,&out);
    cout << out << endl;
  }
  return 0;
ErrorCouldNotOpenFile:
  cout << "Could not open file: " << argv[1] << endl
       << "\t" << strerror(errno) << endl;
  return 1;
}
