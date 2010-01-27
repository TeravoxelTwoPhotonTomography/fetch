//--------------------------------------------------------------------------------------
// File: shader.fx
//
// Copyright (c) Howard Hughes Medical Institute. All rights reserved.
//--------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------
// Constant Buffer Variables
//--------------------------------------------------------------------------------------
Texture3D<snorm float> tx;    // Single image torage for different channels
Texture2D<float4>      cmap;  // each channel is interpreted according to the corresponding colormap
float                  nchan; // the number of channels
                              // ??? [ ] - Who should set nchan?

SamplerState samLinear
{
    Filter = MIN_MAG_MIP_POINT;
    AddressU = Clamp; //Wrap;
    AddressV = Clamp; //Wrap;
};

SamplerState samCmap
{
    Filter = MIN_MAG_MIP_LINEAR;
    AddressU = Clamp;
    AddressV = Clamp;
};

struct VS_INPUT
{
    float4 Pos : POSITION;
    float2 Tex : TEXCOORD;
};

struct PS_INPUT
{
    float4 Pos : SV_POSITION;
    float2 Tex : TEXCOORD0;
};


//--------------------------------------------------------------------------------------
// Vertex Shader
//--------------------------------------------------------------------------------------
PS_INPUT VS( VS_INPUT input )
{   return input;
}


//--------------------------------------------------------------------------------------
// Pixel Shader
//--------------------------------------------------------------------------------------

float4 PS( PS_INPUT input) : SV_Target
{ float  v,z, denom;
  float4 c,
         color = float4(0.0,0.0,0.0,0.0);
         
  if( nchan <= 1.5 )
    denom = nchan;
  else
    denom = nchan - 1.0;
  
  for( float i = 0; i < nchan; i++ )
  { z = i/nchan; //denom;                                                // z is in [0.0,1.0]
    v = tx.Sample( samLinear, float3(input.Tex.x, input.Tex.y, z ) ).x;  // return is in [-1.0,1.0]
    c = cmap.Sample( samCmap, float2( (v+1.0)/2.0, z ) );                // uv is in ( [0.0,1.0], [0.0,1.0] )
    color += c*c.w/nchan;
  }    
  color.w = 1.0;
  return color;
}

float4 PS2( PS_INPUT input) : SV_Target
{ float  v,z, denom;
  float4 c,
         color = float4(0.0,0.0,0.0,0.0);
         
  if( nchan <= 1.5 )
    denom = nchan;
  else
    denom = nchan - 1.0;
  
  z = 0.5; // 0 / (3-1)
  v = tx.Sample( samLinear, float3(input.Tex.x, input.Tex.y, z ) ).x;  // return is in [-1.0,1.0]
  c = cmap.Sample( samCmap, float2( (v+1.0)/2.0, z) );                // uv is in ( [0.0,1.0], [0.0,1.0] )
  color += c; //*c.w;
  
  //z = 1.0; // 0 / (3-1)
  //v = tx.Sample( samLinear, float3(input.Tex.x, input.Tex.y, z ) ).x;  // return is in [-1.0,1.0]
  //c = cmap.Sample( samCmap, float2( (v+1.0)/2.0, z) );                // uv is in ( [0.0,1.0], [0.0,1.0] )
  //color += c; //*c.w;

  color.w = 1.0;
  return color;
}



//--------------------------------------------------------------------------------------
technique10 Render
{
    pass P0
    {
        SetVertexShader( CompileShader( vs_4_0, VS() ) );
        SetGeometryShader( NULL );
        SetPixelShader( CompileShader( ps_4_0, PS() ) );
    }
}

