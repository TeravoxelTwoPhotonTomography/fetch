#pragma once
#include "stdafx.h"
#include <d3d10.h>
#include <d3dx10.h>
#include "types.h"

extern const DXGI_FORMAT g_dx10_tex_type_map[] = {
                            DXGI_FORMAT_R8_UNORM, //  id_u8,
                            DXGI_FORMAT_R16_UNORM,//  id_u16,
                            DXGI_FORMAT_R32_UNORM,//  id_u32,
                            DXGI_FORMAT_UNKNOWN,  //  id_u64,
                            DXGI_FORMAT_R8_SNORM, //  id_i8,
                            DXGI_FORMAT_R16_SNORM,//  id_i16,
                            DXGI_FORMAT_R32_SNORM,//  id_i32,
                            DXGI_FORMAT_UNKNOWN,  //  id_i64,
                            DXGI_FORMAT_R32_FLOAT,//  id_f32,
                            DXGI_FORMAT_UNKNOWN}; //  id_f64

void dx10_effect_variable_set_f32( ID3D10Effect *effect, char *name, f32 val );
f32  dx10_effect_variable_get_f32( ID3D10Effect *effect, char *name );

void dx10_tex2d_dump( char *filename, ID3D10Texture2D *tex );