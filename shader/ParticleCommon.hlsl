#include "Common.hlsl"

TextureCube gCubeMap : register(t0);
Texture2D gShadowMap : register(t1);
Texture2D gTextureMaps[128] : register(t2);

struct VertexIn
{
	float3 PosL    : POSITION;
    float2 SizeW : SIZE;
	float Age : AGE;
    uint Visible : VISIBLE;
};

struct VertexOut{
	float3 CenterW : POSITION;
	float2 SizeW   : SIZE;
    float Age : AGE;
    uint Visible : VISIBLE;
};

struct GeoOut
{
	float4 PosH    : SV_POSITION;
    float3 PosW    : POSITION0;
    float3 NormalW : NORMAL;
    float2 TexC    : TEXCOORD;
    float Age      : AGE;
    uint   PrimID  : SV_PrimitiveID;
};


VertexOut VS(VertexIn vin, uint vertID: SV_VERTEXID){
    VertexOut vout;

    vout.CenterW = mul(float4(vin.PosL, 1.0f), gWorld).xyz;
    vout.SizeW = vin.SizeW;
    vout.Age = vin.Age;
    vout.Visible = vin.Visible;

    return vout;
}

/*calculate the amount the fragment is in shadow*/
float CalcShadowFactor(float4 shadowPosH)
{
    // Complete projection by doing division by w.
    shadowPosH.xyz /= shadowPosH.w;

    // Depth in NDC space.
    float depth = shadowPosH.z;

    uint width, height, numMips;
    gShadowMap.GetDimensions(0, width, height, numMips);

    // Texel size.
    float dx = 1.0f / (float)width;

    float percentLit = 0.0f;
    const float2 offsets[9] =
    {
        float2(-dx,  -dx), float2(0.0f,  -dx), float2(dx,  -dx),
        float2(-dx, 0.0f), float2(0.0f, 0.0f), float2(dx, 0.0f),
        float2(-dx,  +dx), float2(0.0f,  +dx), float2(dx,  +dx)
    };

    [unroll]
    for(int i = 0; i < 9; ++i)
    {
        percentLit += gShadowMap.SampleCmpLevelZero(gsamShadow,
            shadowPosH.xy + offsets[i], depth).r;
    }
    
    return percentLit / 9.0f;
}
