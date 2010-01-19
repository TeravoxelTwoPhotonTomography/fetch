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
    Filter = MIN_MAG_MIP_LINEAR;
    AddressU = Wrap;
    AddressV = Wrap;
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

//float4 PS( PS_INPUT input) : SV_Target
//{   
//    float i = tx0.Sample( samLinear, input.Tex ).x,
//          j = tx1.Sample( samLinear, input.Tex ).x,
//          k = tx2.Sample( samLinear, input.Tex ).x;
//    float4 c0 = cmap0.Sample( samCmap, (i+1.0)/2.0 ),
//           c1 = cmap1.Sample( samCmap, (j+1.0)/2.0 ),
//           c2 = cmap2.Sample( samCmap, (k+1.0)/2.0 );
//
//    return (c0+c1+c2); //additive mixing
//}

float4 PS( PS_INPUT input) : SV_Target
{ float  v,z;
  float4 c,
         color = float4(0.0,0.0,0.0,0.0);
  for( float i = 0; i < nchan; i++ )
  { z = i/nchan;
    v = tx.Sample( samLinear, float3(input.Tex.x, input.Tex.y, z ) ).x;
    c = cmap.Sample( samCmap, float2( (v+1.0)/2.0, z ) );
    color += c*c.w;
  }  

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

