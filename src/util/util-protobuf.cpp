#include <google\protobuf\message.h>
#include "google\protobuf\descriptor.h"
#include "common.h"

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