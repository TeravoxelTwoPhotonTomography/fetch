#include "stdafx.h"
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

template inline Basic_Type_ID TypeID<u8 >(void) {return id_u8; }
template inline Basic_Type_ID TypeID<u16>(void) {return id_u16;}
template inline Basic_Type_ID TypeID<u32>(void) {return id_u32;}
template inline Basic_Type_ID TypeID<u64>(void) {return id_u64;}
template inline Basic_Type_ID TypeID<i8 >(void) {return id_i8; }
template inline Basic_Type_ID TypeID<i16>(void) {return id_i16;}
template inline Basic_Type_ID TypeID<i32>(void) {return id_i32;}
template inline Basic_Type_ID TypeID<i64>(void) {return id_i64;}
template inline Basic_Type_ID TypeID<f32>(void) {return id_f32;}
template inline Basic_Type_ID TypeID<f64>(void) {return id_f64;}

