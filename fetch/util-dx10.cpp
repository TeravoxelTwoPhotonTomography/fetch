#include "stdafx.h"
#include <d3d10.h>
#include <d3dx10.h>

void
dx10_effect_variable_set_f32( ID3D10Effect *effect, char *name, f32 val )
{ Guarded_Assert( SUCCEEDED(
    effect->GetVariableByName(name)->AsScalar()->SetFloat(val)
  ));
}