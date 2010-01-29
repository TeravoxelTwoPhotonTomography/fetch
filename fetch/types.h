#pragma once
#include "stdafx.h"

typedef enum _basic_type_id
{ id_unspecified = -1,
  id_u8 = 0,
  id_u16,
  id_u32,
  id_u64,
  id_i8,
  id_i16,
  id_i32,
  id_i64,
  id_f32,
  id_f64
} Basic_Type_ID;

typedef struct _basic_type_attribute
{ int bytes;
  int bits;
  int is_signed;
  int is_floating;
} Basic_Type_Attribute;

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