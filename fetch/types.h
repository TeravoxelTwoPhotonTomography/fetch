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

extern const Basic_Type_Attribute g_type_attributes[];