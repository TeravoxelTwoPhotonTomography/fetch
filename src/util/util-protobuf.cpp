#include <google\protobuf\message.h>
#include "google\protobuf\descriptor.h"
#include "common.h"
#include "util-protobuf.h"

void set_unset_fields(google::protobuf::Message *msg)
{ 
  const google::protobuf::Descriptor *d = msg->GetDescriptor();
  const google::protobuf::Reflection *r = msg->GetReflection();
  for(int i=0; i<d->field_count();++i)
  { 
    const google::protobuf::FieldDescriptor *field = d->field(i);
    google::protobuf::Message *child = NULL;
    switch(field->type())
    {
    case google::protobuf::FieldDescriptor::TYPE_MESSAGE:
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
        case google::protobuf::FieldDescriptor::CPPTYPE_BOOL:  r->SetBool(msg,field,field->default_value_bool()); break;
        case google::protobuf::FieldDescriptor::CPPTYPE_ENUM:  r->SetEnum(msg,field,field->default_value_enum()); break;
        case google::protobuf::FieldDescriptor::CPPTYPE_DOUBLE:r->SetDouble(msg,field,field->default_value_double()); break;
        case google::protobuf::FieldDescriptor::CPPTYPE_FLOAT: r->SetFloat(msg,field,field->default_value_float()); break;
        case google::protobuf::FieldDescriptor::CPPTYPE_INT32: r->SetInt32(msg,field,field->default_value_int32()); break;                 
        case google::protobuf::FieldDescriptor::CPPTYPE_INT64: r->SetInt64(msg,field,field->default_value_int64()); break;                  
        case google::protobuf::FieldDescriptor::CPPTYPE_UINT32:r->SetUInt32(msg,field,field->default_value_uint32()); break;                  
        case google::protobuf::FieldDescriptor::CPPTYPE_UINT64:r->SetUInt64(msg,field,field->default_value_uint64()); break;                  
        case google::protobuf::FieldDescriptor::CPPTYPE_STRING:r->SetString(msg,field,field->default_value_string()); break;
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

bool equals(const google::protobuf::Message *a,const google::protobuf::Message *b)
{ return equals(*a,*b);
}
bool equals(const google::protobuf::Message &a,const google::protobuf::Message &b)
{
  const google::protobuf::Descriptor 
    *da = a.GetDescriptor(),
    *db = b.GetDescriptor();
  const google::protobuf::Reflection 
    *ra = a.GetReflection(),
    *rb = b.GetReflection();
  
  if( (da->full_name()  !=db->full_name())   // must have same descriptor name
    ||(da->field_count()!=db->field_count()) // and field count
    )
    return false;

  for(int i=0; i<da->field_count();++i)
  { 
    const google::protobuf::FieldDescriptor 
      *afield = da->field(i),
      *bfield = db->field(i);                // indexed in it's order in the proto file, so fields should correspond
    if(  (afield->name()!=bfield->name())    // name's should correspond // ? Neccessary ?
      || (afield->type()!=bfield->type())    // type should correspond
      || (afield->cpp_type()!=bfield->cpp_type())    // type should correspond
      )
      return false;

    if(afield->is_repeated())
    { if(ra->FieldSize(a,afield)!=rb->FieldSize(b,bfield))
        return false;
      for(int j=0; j<ra->FieldSize(a,afield);++j)
        switch(afield->cpp_type())
        {        
          case google::protobuf::FieldDescriptor::CPPTYPE_BOOL:    if( ra->GetRepeatedBool   (a,afield,j)!=rb->GetRepeatedBool   (b,bfield,j) ) return false; break;
          case google::protobuf::FieldDescriptor::CPPTYPE_ENUM:    if( ra->GetRepeatedEnum(a,afield,j)->number()!=rb->GetRepeatedEnum(b,bfield,j)->number() ) return false; break;
          case google::protobuf::FieldDescriptor::CPPTYPE_DOUBLE:  if( fabs(ra->GetRepeatedDouble (a,afield,j)-rb->GetRepeatedDouble (b,bfield,j))>1e-6 ) return false; break;
          case google::protobuf::FieldDescriptor::CPPTYPE_FLOAT:   if( fabs(ra->GetRepeatedFloat  (a,afield,j)-rb->GetRepeatedFloat  (b,bfield,j))>1e-6 ) return false; break;
          case google::protobuf::FieldDescriptor::CPPTYPE_INT32:   if( ra->GetRepeatedInt32  (a,afield,j)!=rb->GetRepeatedInt32  (b,bfield,j) ) return false; break;             
          case google::protobuf::FieldDescriptor::CPPTYPE_INT64:   if( ra->GetRepeatedInt64  (a,afield,j)!=rb->GetRepeatedInt64  (b,bfield,j) ) return false; break;              
          case google::protobuf::FieldDescriptor::CPPTYPE_UINT32:  if( ra->GetRepeatedUInt32 (a,afield,j)!=rb->GetRepeatedUInt32 (b,bfield,j) ) return false; break;                
          case google::protobuf::FieldDescriptor::CPPTYPE_UINT64:  if( ra->GetRepeatedUInt64 (a,afield,j)!=rb->GetRepeatedUInt64 (b,bfield,j) ) return false; break;                
          case google::protobuf::FieldDescriptor::CPPTYPE_STRING:  if( ra->GetRepeatedString (a,afield,j)!=rb->GetRepeatedString (b,bfield,j) ) return false; break;
          case google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE: if( ra->GetRepeatedMessage(a,afield,j)!=rb->GetRepeatedMessage(b,bfield,j) ) return false; break;
          default:
            error("Got unexpected CppType\n");
        }
    } else // => not repeated
    {   switch(afield->cpp_type())
        {        
          case google::protobuf::FieldDescriptor::CPPTYPE_BOOL:    if( ra->GetBool   (a,afield)!=rb->GetBool   (b,bfield) ) return false; break;
          case google::protobuf::FieldDescriptor::CPPTYPE_ENUM:    if( ra->GetEnum(a,afield)->number()!=rb->GetEnum(b,bfield)->number() ) return false; break;
          case google::protobuf::FieldDescriptor::CPPTYPE_DOUBLE:  if( fabs(ra->GetDouble (a,afield)-rb->GetDouble (b,bfield))>1e-6 ) return false; break;
          case google::protobuf::FieldDescriptor::CPPTYPE_FLOAT:   if( fabs(ra->GetFloat  (a,afield)-rb->GetFloat  (b,bfield))>1e-6 ) return false; break;
          case google::protobuf::FieldDescriptor::CPPTYPE_INT32:   if( ra->GetInt32  (a,afield)!=rb->GetInt32  (b,bfield) ) return false; break;             
          case google::protobuf::FieldDescriptor::CPPTYPE_INT64:   if( ra->GetInt64  (a,afield)!=rb->GetInt64  (b,bfield) ) return false; break;              
          case google::protobuf::FieldDescriptor::CPPTYPE_UINT32:  if( ra->GetUInt32 (a,afield)!=rb->GetUInt32 (b,bfield) ) return false; break;                
          case google::protobuf::FieldDescriptor::CPPTYPE_UINT64:  if( ra->GetUInt64 (a,afield)!=rb->GetUInt64 (b,bfield) ) return false; break;                
          case google::protobuf::FieldDescriptor::CPPTYPE_STRING:  if( ra->GetString (a,afield)!=rb->GetString (b,bfield) ) return false; break;
#undef GetMessage // Windows shenanigans
          case google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE: if( ra->GetMessage(a,afield)!=rb->GetMessage(b,bfield) ) return false; break;
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
