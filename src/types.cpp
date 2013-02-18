
#include "types.h"

extern const
Basic_Type_Attribute g_type_attributes[] = {
          /*Bpp   signed?   floating? */
          {   1, 8,    0,           0 } ,  //id_u8
          {   2,16,    0,           0 } ,  //id_u16
          {   4,32,    0,           0 } ,  //id_u32
          {   8,64,    0,           0 } ,  //id_u64
          {   1, 8,    1,           0 } ,  //id_i8
          {   2,16,    1,           0 } ,  //id_i16
          {   4,32,    1,           0 } ,  //id_i32
          {   8,64,    1,           0 } ,  //id_i64
          {   4,32,    1,           1 } ,  //id_f32
          {   8,64,    1,           1 }};  //id_f64



//template<> inline Basic_Type_ID TypeID<char>(void) {return id_i8; }
//template<> inline Basic_Type_ID TypeID<short>(void) {return id_i16;}

const char *TypeStrFromID(Basic_Type_ID id)
{
  switch(id)
  { 
    case id_u8 : return TypeStr<u8 >();
    case id_u16: return TypeStr<u16>(); 
    case id_u32: return TypeStr<u32>();
    case id_u64: return TypeStr<u64>();
    case id_i8 : return TypeStr<i8 >();
    case id_i16: return TypeStr<i16>();
    case id_i32: return TypeStr<i32>();
    case id_i64: return TypeStr<i64>();
    case id_f32: return TypeStr<f32>();
    case id_f64: return TypeStr<f64>();
    default:
      error("Unrecognized type id (id=%d).\r\n",id);
      return "error";
    }
}
