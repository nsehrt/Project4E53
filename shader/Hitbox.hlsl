#include "DefaultCommon.hlsl"

struct VertexIn
{
	float3 PosL    : POSITION;
};

/*only homogeneous clip space needed for this shader*/
struct VertexOut
{
	float4 PosH    : SV_POSITION;
}; 

/*ignore everything but the position*/
VertexOut VS(VertexIn vin)
{
	VertexOut vout = (VertexOut)0.0f;

    float4 posW = mul(float4(vin.PosL, 1.0f), gWorld);
    vout.PosH = mul(posW, gViewProj);
	
    return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
    /*just return pink color*/
    return float4(0.8f, 0.26f, 0.95f, 1.0f);
}