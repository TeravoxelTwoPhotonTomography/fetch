//--------------------------------------------------------------------------------------
// File: Tutorial07.fx
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------
// Constant Buffer Variables
//--------------------------------------------------------------------------------------
Texture2D txR;
Texture2D txG;
Texture2D txB;

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
    float r = txR.Sample( samLinear, input.Tex ).x,
          g = txG.Sample( samLinear, gtex ).x,
          b = txB.Sample( samLinear, btex ).x;

    return float4(r,g,b,1.0);
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

