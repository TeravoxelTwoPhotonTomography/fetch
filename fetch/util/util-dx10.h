#pragma once
#include "stdafx.h"
#include <d3d10.h>
#include <d3dx10.h>
#include "types.h"

extern const DXGI_FORMAT g_dx10_tex_type_map[];

void dx10_effect_variable_set_f32( ID3D10Effect *effect, char *name, f32 val );
f32  dx10_effect_variable_get_f32( ID3D10Effect *effect, char *name );

void dx10_tex2d_dump( char *filename, ID3D10Texture2D *tex );