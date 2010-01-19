#include "stdafx.h"
#include "render-colormap.h"

#define CLAMP(v,low,high) ((v)<(low))?(low):(((v)>(high))?(high):(v))

Colormap_Resource*
Colormap_Resource_Alloc (void)
{ return (Colormap_Resource*) Guarded_Calloc( sizeof(Colormap_Resource), 1, "Colormap_Resource_Alloc");
}

void
Colormap_Resource_Free(Colormap_Resource *cmap)
{ if(cmap)
  { //ensure detached
    if(cmap->texture) // then the others got attached too
      Colormap_Resource_Detach(cmap);
    free(cmap);
  }
}

inline void 
_create_texture(ID3D10Device *device, ID3D10Texture1D **pptex, UINT width, UINT nchan )
{ D3D10_TEXTURE1D_DESC desc;
  ZeroMemory( &desc, sizeof(desc) );  
  desc.Width                         = width;
  desc.MipLevels                     = 1;
  desc.Format                        = DXGI_FORMAT_R32G32B32A32_FLOAT;  
  desc.Usage                         = D3D10_USAGE_DYNAMIC;
  desc.CPUAccessFlags                = D3D10_CPU_ACCESS_WRITE;
  desc.BindFlags                     = D3D10_BIND_SHADER_RESOURCE;
  desc.ArraySize                     = nchan;
  
  Guarded_Assert( width > 0 );
  Guarded_Assert( nchan > 0 );
  Guarded_Assert(SUCCEEDED(
    device->CreateTexture1D( &desc, NULL, pptex ) ));
}

inline void
_bind_texture_to_shader_variable(ID3D10Device *device, Colormap_Resource *cmap )
{ D3D10_SHADER_RESOURCE_VIEW_DESC srvDesc;
  D3D10_RESOURCE_DIMENSION        type;
  D3D10_TEXTURE1D_DESC            desc;
  ID3D10Texture1D                *tex = cmap->texture;
  
  tex->GetDesc( &desc );
  tex->GetType( &type );
  Guarded_Assert( type == D3D10_RESOURCE_DIMENSION_TEXTURE1D );

  srvDesc.Format                    = desc.Format;
  srvDesc.ViewDimension             = D3D10_SRV_DIMENSION_TEXTURE1DARRAY;
  srvDesc.Texture1DArray.MipLevels       = desc.MipLevels;
  srvDesc.Texture1DArray.MostDetailedMip = desc.MipLevels - 1;
  srvDesc.Texture1DArray.FirstArraySlice = 0;
  srvDesc.Texture1DArray.ArraySize       = desc.ArraySize;

  Guarded_Assert(SUCCEEDED( 
    device->CreateShaderResourceView( tex,                                   // Make the shader resource view
                                     &srvDesc, 
                                     &cmap->resource_view ) ));                                               
  Guarded_Assert(SUCCEEDED(
    cmap->resource_variable->SetResource( cmap->resource_view ) ));          // Bind
}

void
Colormap_Resource_Attach (Colormap_Resource *cmap, UINT width, UINT nchan, ID3D10Effect *effect, const char* name)
{ ID3D10Device *device = NULL;
  Guarded_Assert(SUCCEEDED( effect->GetDevice(&device) ));
  
  cmap->resource_variable = effect->GetVariableByName(name)->AsShaderResource();
  _create_texture( device, &cmap->texture, width, nchan );
  _bind_texture_to_shader_variable( device, cmap );
  device->Release();
  
  cmap->nchan = nchan;
}

void
Colormap_Resource_Detach (Colormap_Resource *cmap)
{ if( cmap->resource_view) cmap->resource_view->Release();
  if( cmap->texture) cmap->texture->Release();
  cmap->resource_view = NULL;
  cmap->texture       = NULL;
}

UINT
Colormap_Resource_Get_Element_Count(Colormap_Resource *cmap)
{ D3D10_TEXTURE1D_DESC    desc;
  cmap->texture->GetDesc(&desc);
  return desc.Width;
}

void
Colormap_Resource_Fill (Colormap_Resource *cmap, UINT ichan, f32 *bytes, size_t nbytes)
{ void *data;
  ID3D10Texture1D       *dst = cmap->texture;
  Guarded_Assert(SUCCEEDED( 
      dst->Map( D3D10CalcSubresource(0, ichan, 1), D3D10_MAP_WRITE_DISCARD, 0, &data ) ));  
  memcpy(data, bytes, nbytes);
  dst->Unmap( D3D10CalcSubresource(0, ichan, 1) );        
}

f32 *_linear_colormap_get_params( float x0, float x1, float sign, size_t nrows, float *slope, float *intercept, size_t *nbytes )
// Sizes static colormap buffer and computes slope and intercept
{ float m   =  sign/(x1*nrows-x0*nrows),
        b   = -m*x0*nrows;
  static f32 *rgba = NULL;
  static size_t n  = 0, sz;
  
  if(!rgba)               // alloc
  { sz = nrows*4*sizeof(f32);
    rgba = (f32*) Guarded_Malloc(sz , "_linear_colormap_params" );
  } else if( n != nrows ) // realloc
  { sz = nrows*4*sizeof(f32);
    Guarded_Realloc( (void**) &rgba, sz , "_linear_colormap_params" );
  }
  *nbytes    = sz;
  *slope     = m;
  *intercept = b;
  return rgba;
}

void Colormap_Black( Colormap_Resource *cmap, UINT ichan )
{ void *data = NULL;
  UINT width = Colormap_Resource_Get_Element_Count(cmap);
  Guarded_Assert( !FAILED( 
      cmap->texture->Map( D3D10CalcSubresource(0, ichan, 1), D3D10_MAP_WRITE_DISCARD, 0, &data ) ));  
  memset(data, 0, 4*width*sizeof(f32) );
  cmap->texture->Unmap( D3D10CalcSubresource(0, ichan, 1) );
}

void
Colormap_Gray( Colormap_Resource *cmap, UINT ichan, float min, float max )
{ size_t nbytes;
  UINT nrows = Colormap_Resource_Get_Element_Count(cmap);
  float m,b,
       *rgba = _linear_colormap_get_params( min, max, 1.0f, nrows, &m, &b, &nbytes );
  while(nrows--)
  { f32 *row = rgba + nrows*4;
    f32    v = m*nrows + b;    
    row[0] = row[1] = row[2] = CLAMP(v,0.0f,1.0f);
    row[3] = 1.0f;
  }      
  Colormap_Resource_Fill( cmap, ichan, rgba, nbytes );      
}

void Colormap_Inverse_Gray( Colormap_Resource *cmap, UINT ichan, float min, float max )
{ size_t nbytes;
  UINT nrows = Colormap_Resource_Get_Element_Count(cmap);
  float m,b,
       *rgba = _linear_colormap_get_params( max, min, 1.0f, nrows, &m, &b, &nbytes );
  while(nrows--)
  { f32 *row = rgba + 4*nrows;
    f32    v = m*nrows + b;
    row[0] = row[1] = row[2] = CLAMP(v,0.0f,1.0f);
    row[3] = 1.0f;
  }      
  Colormap_Resource_Fill( cmap, ichan, rgba, nbytes );
}

inline void 
_colormap_single_channel( Colormap_Resource *cmap, UINT ichan, float min, float max, int channel )
{ size_t nbytes;
  UINT nrows = Colormap_Resource_Get_Element_Count(cmap);
  float m,b,
       *rgba = _linear_colormap_get_params( min, max, 1.0f, nrows, &m, &b, &nbytes );
  memset(rgba,0,nbytes);
  while(nrows--)
  { f32 *row = rgba + nrows*4;
    f32    v = m*nrows + b;    
    row[channel] = CLAMP(v,0.0f,1.0f);
    row[3] = 1.0f;
  }
  Colormap_Resource_Fill( cmap, ichan, rgba, nbytes );
}

void
Colormap_Red ( Colormap_Resource *cmap, UINT ichan, float min, float max )
{ _colormap_single_channel( cmap, ichan, min, max, 0 );
}

void
Colormap_Green ( Colormap_Resource *cmap, UINT ichan, float min, float max )
{ _colormap_single_channel( cmap, ichan, min, max, 1 );
}

void
Colormap_Blue ( Colormap_Resource *cmap, UINT ichan, float min, float max )
{ _colormap_single_channel( cmap, ichan, min, max, 2 );
}

void
Colormap_Alpha ( Colormap_Resource *cmap, UINT ichan, float min, float max )
{ _colormap_single_channel( cmap, ichan, min, max, 3 );
}

inline 
void _convert_single_hsva_to_rgba( f32 *hsva, f32* rgba)
{ f32 H = 6.0f* hsva[0], V = hsva[2],
      S = hsva[1]          , A = hsva[3];
  f32 f,p,ih;
                         // v  t/q  p
  static int idx[][3] = { { 0 , 1 , 2 }, // 0 - t
                          { 1 , 0 , 2 }, // 1 - q
                          { 1 , 2 , 0 }, // 2 - t
                          { 2 , 1 , 0 }, // 3 - q
                          { 2 , 0 , 1 }, // 4 - t
                          { 0 , 2 , 1 }, // 5 - q
                          { 0 , 1 , 2 }}; // 6==0 - t
  static f32 q,t,
            *qt[] = {&t, &q};
  
  ih = floorf(H);
  f  = H - ih;
  p  = V * (1.0f -       S);
  q  = V * (1.0f -    f *S);
  t  = V * (1.0f - (1-f)*S);
  
  { unsigned int i = (unsigned int) ih;
    rgba[ idx[i][0] ] = V;
    rgba[ idx[i][1] ] = *qt[i&1];
    rgba[ idx[i][2] ] = p;
    rgba[ 3         ] = A;
  }
}

inline 
void _convert_array_hsva_to_rgba( f32 *hsva, f32* rgba, size_t nrows )
// all hsva and rbga values are in [0,1]
{ f32 *sb = hsva,
      *s  = sb + 4*nrows,
      *db = rgba,
      *d  = db + 4*nrows;
  while( (s-=4) >= sb )
  { d-=4;
    _convert_single_hsva_to_rgba(s,d);
  }
}

inline void
_set_channel_to_constant( f32 *rgba, size_t nrows, int channel, f32 v )
{ f32 *beg = rgba, *cur = rgba + 4*nrows;
  while((cur-=4) >= beg)
    cur[channel] = v;
}

inline void
_set_channel_linear_ramp( f32 *rgba, size_t nrows, int channel, f32 m, f32 b )
{ while(nrows--)
  { f32 v =  m * nrows + b; 
    rgba[channel+4*nrows] = CLAMP(v,0.0f,1.0f);
  }
} 

void Colormap_HSV_Hue( Colormap_Resource *cmap, UINT ichan, float S, float V, float A, float min, float max )
{ size_t nbytes;
  UINT nrows = Colormap_Resource_Get_Element_Count(cmap);
  float m,b,
       *hsva = _linear_colormap_get_params( min, max, 1.0f, nrows, &m, &b, &nbytes );
  _set_channel_linear_ramp( hsva, nrows, 0, m, b );
  _set_channel_to_constant( hsva, nrows, 1, S );
  _set_channel_to_constant( hsva, nrows, 2, V );
  _set_channel_to_constant( hsva, nrows, 3, A );
  
  _convert_array_hsva_to_rgba(hsva,hsva,nrows);
  Colormap_Resource_Fill( cmap, ichan, hsva, nbytes );
}

void Colormap_HSV_Hue_Value( Colormap_Resource *cmap, UINT ichan, float S, float A, float min, float max )
{ size_t nbytes;
  UINT nrows = Colormap_Resource_Get_Element_Count(cmap);
  float m,b,
       *hsva = _linear_colormap_get_params( min, max, 1.0f, nrows, &m, &b, &nbytes );
  _set_channel_linear_ramp( hsva, nrows, 0, m, b );
  _set_channel_to_constant( hsva, nrows, 1, S );
  _set_channel_linear_ramp( hsva, nrows, 2, m, b );  
  _set_channel_to_constant( hsva, nrows, 3, A );
  
  _convert_array_hsva_to_rgba(hsva,hsva,nrows);
  Colormap_Resource_Fill( cmap, ichan, hsva, nbytes );
}

void Colormap_HSV_Saturation( Colormap_Resource *cmap, UINT ichan, float H, float V, float A, float min, float max )
{ size_t nbytes;
  UINT nrows = Colormap_Resource_Get_Element_Count(cmap);
  float m,b,
       *hsva = _linear_colormap_get_params( min, max, 1.0f, nrows, &m, &b, &nbytes );  
  _set_channel_to_constant( hsva, nrows, 0, H );
  _set_channel_linear_ramp( hsva, nrows, 1, m, b );
  _set_channel_to_constant( hsva, nrows, 2, V );
  _set_channel_to_constant( hsva, nrows, 3, A );
  
  _convert_array_hsva_to_rgba(hsva,hsva,nrows);
  Colormap_Resource_Fill( cmap, ichan, hsva, nbytes );
}

void Colormap_HSV_Value( Colormap_Resource *cmap, UINT ichan, float H, float S, float A, float min, float max )
{ size_t nbytes;
  UINT nrows = Colormap_Resource_Get_Element_Count(cmap);
  float m,b,
       *hsva = _linear_colormap_get_params( min, max, 1.0f, nrows, &m, &b, &nbytes );
  _set_channel_to_constant( hsva, nrows, 0, H );
  _set_channel_to_constant( hsva, nrows, 1, S );
  _set_channel_linear_ramp( hsva, nrows, 2, m, b );
  _set_channel_to_constant( hsva, nrows, 3, A );
  
  _convert_array_hsva_to_rgba(hsva,hsva,nrows);
  Colormap_Resource_Fill( cmap, ichan, hsva, nbytes );
}

void Colormap_HSV_Alpha( Colormap_Resource *cmap, UINT ichan, float H, float S, float V, float min, float max )
{ size_t nbytes;
  UINT nrows = Colormap_Resource_Get_Element_Count(cmap);
  float m,b,
       *hsva = _linear_colormap_get_params( min, max, 1.0f, nrows, &m, &b, &nbytes );
  _set_channel_to_constant( hsva, nrows, 0, H );
  _set_channel_to_constant( hsva, nrows, 1, S );
  _set_channel_to_constant( hsva, nrows, 2, V );
  _set_channel_linear_ramp( hsva, nrows, 3, m, b );
  
  _convert_array_hsva_to_rgba(hsva,hsva,nrows);
  Colormap_Resource_Fill( cmap, ichan, hsva, nbytes );
}

//
// Multichannel Colormap functions
// -------------------------------
//

void
Colormap_Autosetup( Colormap_Resource *cmap, float *min, float *max )
{ UINT N = cmap->nchan;
  Guarded_Assert( N );
  if( N == 1)
  { Colormap_Gray(cmap,0,min[0],max[0]);    
  } else
  { int ichan = N;
    while( ichan-- )                       /* hue                val  alpha */
      Colormap_HSV_Saturation(cmap, ichan, ichan/((float)N), 1.0, 1.0, min[ichan], max[ichan] );
  }
  return;
}