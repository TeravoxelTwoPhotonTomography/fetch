#pragma once
#include "common.h"

#include <limits.h>
#include <float.h>

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
  id_f64,
  MAX_TYPE_ID
} Basic_Type_ID;

typedef struct _basic_type_attribute
{ int bytes;
  int bits;
  int is_signed;
  int is_floating;
} Basic_Type_Attribute;

extern const Basic_Type_Attribute g_type_attributes[];

#define TYPE_IS_SIGNED(id)   (g_type_attributes[id].is_signed)
#define TYPE_IS_FLOATING(id) (g_type_attributes[id].is_floating)
#define TYPE_NBITS(id)       (g_type_attributes[id].bits)
#define TYPE_NBYTES(id)      (g_type_attributes[id].bytes)

//
// TypeID<T>();
//

template<typename T> inline Basic_Type_ID TypeID(void);

template<> inline Basic_Type_ID TypeID<u8 >(void) {return id_u8; }
template<> inline Basic_Type_ID TypeID<u16>(void) {return id_u16;}
template<> inline Basic_Type_ID TypeID<u32>(void) {return id_u32;}
template<> inline Basic_Type_ID TypeID<u64>(void) {return id_u64;}
template<> inline Basic_Type_ID TypeID<i8 >(void) {return id_i8; }
template<> inline Basic_Type_ID TypeID<i16>(void) {return id_i16;}
template<> inline Basic_Type_ID TypeID<i32>(void) {return id_i32;}
template<> inline Basic_Type_ID TypeID<i64>(void) {return id_i64;}
template<> inline Basic_Type_ID TypeID<f32>(void) {return id_f32;}
template<> inline Basic_Type_ID TypeID<f64>(void) {return id_f64;}

//
// To string
//

const char *TypeStrFromID(Basic_Type_ID id);

template<typename T> inline const char* TypeStr(void);
template<> inline const char* TypeStr<u8 >(void) {return "u8"; }
template<> inline const char* TypeStr<u16>(void) {return "u16";}
template<> inline const char* TypeStr<u32>(void) {return "u32";}
template<> inline const char* TypeStr<u64>(void) {return "u64";}
template<> inline const char* TypeStr<i8 >(void) {return "i8"; }
template<> inline const char* TypeStr<i16>(void) {return "i16";}
template<> inline const char* TypeStr<i32>(void) {return "i32";}
template<> inline const char* TypeStr<i64>(void) {return "i64";}
template<> inline const char* TypeStr<f32>(void) {return "f32";}
template<> inline const char* TypeStr<f64>(void) {return "f64";}

//
// Saturation
//

template<typename T> inline T TypeMax();
template<> inline u8  TypeMax<u8 >() {return UCHAR_MAX;}
template<> inline u16 TypeMax<u16>() {return USHRT_MAX;}
template<> inline u32 TypeMax<u32>() {return UINT_MAX;}
template<> inline u64 TypeMax<u64>() {return ULLONG_MAX;}
template<> inline i8  TypeMax<i8 >() {return  CHAR_MAX;}
template<> inline i16 TypeMax<i16>() {return  SHRT_MAX;}
template<> inline i32 TypeMax<i32>() {return  INT_MAX;}
template<> inline i64 TypeMax<i64>() {return  LLONG_MAX;}
template<> inline f32 TypeMax<f32>() {return  FLT_MAX;}
template<> inline f64 TypeMax<f64>() {return  DBL_MAX;}

template<typename T> inline T TypeMin();
template<> inline u8  TypeMin<u8 >() {return 0;}
template<> inline u16 TypeMin<u16>() {return 0;}
template<> inline u32 TypeMin<u32>() {return 0;}
template<> inline u64 TypeMin<u64>() {return 0;}
template<> inline i8  TypeMin<i8 >() {return  CHAR_MIN;}
template<> inline i16 TypeMin<i16>() {return  SHRT_MIN;}
template<> inline i32 TypeMin<i32>() {return  INT_MIN;}
template<> inline i64 TypeMin<i64>() {return  LLONG_MIN;}
template<> inline f32 TypeMin<f32>() {return -FLT_MAX;}
template<> inline f64 TypeMin<f64>() {return -DBL_MAX;}

template<typename Tdst,typename Tsrc> inline Tdst Saturate(Tsrc x) 
{ 
#pragma warning(push)
#pragma warning(disable:4244 4018)
  return (x>TypeMax<Tdst>())?TypeMax<Tdst>():((x<TypeMin<Tdst>())?TypeMin<Tdst>():x); 
#pragma warning(pop)  
}