//--------------------------------------------------------------------------------------
// File: Tutorial07.fx
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------
// Constant Buffer Variables
//--------------------------------------------------------------------------------------
Texture2D<unorm float> tx0;
Texture2D<unorm float> tx1;
Texture2D<unorm float> tx2;

Texture1D<unorm float4> cmap0;
Texture1D<unorm float4> cmap1;
Texture1D<unorm float4> cmap2;

SamplerState samLinear
{
    Filter = MIN_MAG_MIP_POINT;
    AddressU = Wrap;
    AddressV = Wrap;
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
    float4 c0 = cmap0.Sample( samLinear, i ),
           c1 = cmap1.Sample( samLinear, j ),
           c2 = cmap2.Sample( samLinear, k );

    return c0+c1+c2; //additive mixing
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

