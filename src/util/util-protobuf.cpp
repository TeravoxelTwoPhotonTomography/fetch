#include <google\protobuf\message.h>
#include "google\protobuf\descriptor.h"
#include "common.h"
#include "util-protobuf.h"
#include <math.h>

void set_unset_fields(google::protobuf::Message *msg)
{ namespace GPB = google::protobuf;
  typedef GPB::FieldDescriptor FD;
  const GPB::Descriptor *d = msg->GetDescriptor();
  const GPB::Reflection *r = msg->GetReflection();
  for(int i=0; i<d->field_count();++i)
  { 
    const FD *field = d->field(i);
    GPB::Message *child = NULL;
    switch(field->type())
    {
    case FD::TYPE_MESSAGE:
      if(field->is_repeated())
      { 
        for(int j=0; j<r->FieldSize(*msg,field);++j)
        {
          child = r->MutableRepeatedMessage(msg,field,j);
          set_unset_fields(child);
        }

      } else
      {
        child = r->MutableMessage(msg,field,NULL);           
        set_unset_fields(child);
      }
      break;
    default:
      if(field->has_default_value() && !r->HasField(*msg,field))
      { 
        switch(field->cpp_type())
        {
        case FD::CPPTYPE_BOOL:  r->SetBool(msg,field,field->default_value_bool()); break;
        case FD::CPPTYPE_ENUM:  r->SetEnum(msg,field,field->default_value_enum()); break;
        case FD::CPPTYPE_DOUBLE:r->SetDouble(msg,field,field->default_value_double()); break;
        case FD::CPPTYPE_FLOAT: r->SetFloat(msg,field,field->default_value_float()); break;
        case FD::CPPTYPE_INT32: r->SetInt32(msg,field,field->default_value_int32()); break;                 
        case FD::CPPTYPE_INT64: r->SetInt64(msg,field,field->default_value_int64()); break;                  
        case FD::CPPTYPE_UINT32:r->SetUInt32(msg,field,field->default_value_uint32()); break;                  
        case FD::CPPTYPE_UINT64:r->SetUInt64(msg,field,field->default_value_uint64()); break;                  
        case FD::CPPTYPE_STRING:r->SetString(msg,field,field->default_value_string()); break;
        default:
          warning("Got unexpected CppType\n");
        }
      }
    }        
  }
}

bool equals(const google::protobuf::Message &a,const google::protobuf::Message &b);
bool operator!=(const google::protobuf::Message &a,const google::protobuf::Message &b)
{ return !equals(a,b);
}

#if 0 // Kit for debugging equals() method
void onfalse() 
{ debug("breakme"ENDL);
}
#define REPORT HERE; onfalse()
#else
#define REPORT
#endif

bool equals(const google::protobuf::Message *a,const google::protobuf::Message *b)
{ return equals(*a,*b);
}
bool equals(const google::protobuf::Message &a,const google::protobuf::Message &b)
{ namespace GPB = google::protobuf;
  const GPB::Descriptor 
    *da = a.GetDescriptor(),
    *db = b.GetDescriptor();
  const GPB::Reflection 
    *ra = a.GetReflection(),
    *rb = b.GetReflection();
  
  if( (da->full_name()  !=db->full_name())   // must have same descriptor name
    ||(da->field_count()!=db->field_count()) // and field count
    )
    {REPORT; return false;}

  for(int i=0; i<da->field_count();++i)
  { typedef GPB::FieldDescriptor FD;
    const FD 
      *afield = da->field(i),
      *bfield = db->field(i);                // indexed in it's order in the proto file, so fields should correspond
    if(  (afield->name()!=bfield->name())    // name's should correspond // ? Neccessary ?
      || (afield->type()!=bfield->type())    // type should correspond
      || (afield->cpp_type()!=bfield->cpp_type())    // type should correspond
      )    
    { REPORT; return false;}

    if(afield->is_repeated())
    { if(ra->FieldSize(a,afield)!=rb->FieldSize(b,bfield))
        { REPORT; return false;}
      for(int j=0; j<ra->FieldSize(a,afield);++j)
        switch(afield->cpp_type())
        {        
          case FD::CPPTYPE_BOOL:    if( ra->GetRepeatedBool   (a,afield,j)!=rb->GetRepeatedBool   (b,bfield,j) ) { REPORT; return false;} break;
          case FD::CPPTYPE_ENUM:    if( ra->GetRepeatedEnum(a,afield,j)->number()!=rb->GetRepeatedEnum(b,bfield,j)->number() ) {REPORT; return false;} break;
          case FD::CPPTYPE_DOUBLE:  if( fabs(ra->GetRepeatedDouble (a,afield,j)-rb->GetRepeatedDouble (b,bfield,j))>1e-6 ) {REPORT; return false;} break;
          case FD::CPPTYPE_FLOAT:   if( fabs(ra->GetRepeatedFloat  (a,afield,j)-rb->GetRepeatedFloat  (b,bfield,j))>1e-6 ) {REPORT; return false;} break;
          case FD::CPPTYPE_INT32:   if( ra->GetRepeatedInt32  (a,afield,j)!=rb->GetRepeatedInt32  (b,bfield,j) ) {REPORT; return false;} break;             
          case FD::CPPTYPE_INT64:   if( ra->GetRepeatedInt64  (a,afield,j)!=rb->GetRepeatedInt64  (b,bfield,j) ) {REPORT; return false;} break;              
          case FD::CPPTYPE_UINT32:  if( ra->GetRepeatedUInt32 (a,afield,j)!=rb->GetRepeatedUInt32 (b,bfield,j) ) {REPORT; return false;} break;                
          case FD::CPPTYPE_UINT64:  if( ra->GetRepeatedUInt64 (a,afield,j)!=rb->GetRepeatedUInt64 (b,bfield,j) ) {REPORT; return false;} break;                
          case FD::CPPTYPE_STRING:  if( ra->GetRepeatedString (a,afield,j)!=rb->GetRepeatedString (b,bfield,j) ) {REPORT; return false;} break;
          case FD::CPPTYPE_MESSAGE: if( ra->GetRepeatedMessage(a,afield,j)!=rb->GetRepeatedMessage(b,bfield,j) ) {REPORT; return false;} break;
          default:
            error("Got unexpected CppType\n");
        }
    } else // => not repeated
    {   switch(afield->cpp_type())
        {        
          case FD::CPPTYPE_BOOL:    if( ra->GetBool   (a,afield)!=rb->GetBool   (b,bfield) ) {REPORT; return false;} break;
          case FD::CPPTYPE_ENUM:    if( ra->GetEnum(a,afield)->number()!=rb->GetEnum(b,bfield)->number() ) {REPORT; return false;} break;
          case FD::CPPTYPE_DOUBLE:  if( fabs(ra->GetDouble (a,afield)-rb->GetDouble (b,bfield))>1e-6 ) {REPORT; return false;} break;
          case FD::CPPTYPE_FLOAT:   if( fabs(ra->GetFloat  (a,afield)-rb->GetFloat  (b,bfield))>1e-6 ) {REPORT; return false;} break;
          case FD::CPPTYPE_INT32:   if( ra->GetInt32  (a,afield)!=rb->GetInt32  (b,bfield) ) {REPORT; return false;} break;             
          case FD::CPPTYPE_INT64:   if( ra->GetInt64  (a,afield)!=rb->GetInt64  (b,bfield) ) {REPORT; return false;} break;              
          case FD::CPPTYPE_UINT32:  if( ra->GetUInt32 (a,afield)!=rb->GetUInt32 (b,bfield) ) {REPORT; return false;} break;                
          case FD::CPPTYPE_UINT64:  if( ra->GetUInt64 (a,afield)!=rb->GetUInt64 (b,bfield) ) {REPORT; return false;} break;                
          case FD::CPPTYPE_STRING:  if( ra->GetString (a,afield)!=rb->GetString (b,bfield) ) {REPORT; return false;} break;
#undef GetMessage // Windows shenanigans
          case FD::CPPTYPE_MESSAGE: if( ra->GetMessage(a,afield)!=rb->GetMessage(b,bfield) ) {REPORT; return false;} break;
          default:
            error("Got unexpected CppType\n");
        }
    }                   
  } // end for loop over fields
  return true;
}

struct CS
{
  CRITICAL_SECTION _cs;
  CS()  {InitializeCriticalSectionAndSpinCount( &_cs, 0x80000400 );}
  ~CS() {DeleteCriticalSection(&_cs);}

  inline void lock()    {EnterCriticalSection(&_cs);}
  inline void unlock()  {LeaveCriticalSection(&_cs);}
};

namespace pb 
{
  CS _g_cs;
  void lock()   {_g_cs.lock();}
  void unlock() {_g_cs.unlock();}
} //end namespace pb
