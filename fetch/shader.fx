//--------------------------------------------------------------------------------------
// File: shader.fx
//
// Copyright (c) Howard Hughes Medical Institute. All rights reserved.
//--------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------
// Constant Buffer Variables
//--------------------------------------------------------------------------------------
Texture2D<snorm float> tx0;
Texture2D<snorm float> tx1;
Texture2D<snorm float> tx2;

Texture1D<float4> cmap0;
Texture1D<float4> cmap1;
Texture1D<float4> cmap2;

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

cbuffer cbChangesEveryFrame
{
    float4 vMeshColor;
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
{   float  s =  sin(3.14159/4),
           c =  cos(3.14159/4);
    float2 gtex = float2( input.Tex.y, input.Tex.x ),
           btex = float2( s*input.Tex.x + c*input.Tex.y, s*input.Tex.x - c*input.Tex.y );
    float i = tx0.Sample( samLinear, input.Tex ).x,
          j = tx1.Sample( samLinear, gtex ).x,
          k = tx2.Sample( samLinear, btex ).x;

    return float4(i,j,k,1.0);
}

float4 PS_w_cmap( PS_INPUT input) : SV_Target
{   
//    float  s =  sin(3.14159/4),
//           c =  cos(3.14159/4);
//    float2 gtex = float2( input.Tex.y, input.Tex.x ),
//           btex = float2( s*input.Tex.x + c*input.Tex.y, s*input.Tex.x - c*input.Tex.y );
    float i = tx0.Sample( samLinear, input.Tex ).x,
          j = tx1.Sample( samLinear, input.Tex ).x,
          k = tx2.Sample( samLinear, input.Tex ).x;
    float4 c0 = cmap0.Sample( samCmap, (i+1.0)/2.0 ),
           c1 = cmap1.Sample( samCmap, (j+1.0)/2.0 ),
           c2 = cmap2.Sample( samCmap, (k+1.0)/2.0 );

    return (c0+c1+c2); //additive mixing
}


//--------------------------------------------------------------------------------------
technique10 Render
{
    pass P0
    {
        SetVertexShader( CompileShader( vs_4_0, VS() ) );
        SetGeometryShader( NULL );
        SetPixelShader( CompileShader( ps_4_0, PS_w_cmap() ) );
    }
}

