#include "Common.hlsl"

#define SHADOW_ADD_BRIGHTNESS 0.4f

struct VertexIn
{
	float3 PosW  : POSITION;
	float2 SizeW : SIZE;
};

struct VertexOut
{
	float3 CenterW : POSITION;
	float2 SizeW   : SIZE;
};

struct GeoOut
{
	float4 PosH    : SV_POSITION;
    float3 PosW    : POSITION;
    float3 NormalW : NORMAL;
    float2 TexC    : TEXCOORD;
    uint   PrimID  : SV_PrimitiveID;
};


VertexOut VS(VertexIn vin, uint vertID: SV_VERTEXID){
    VertexOut vout;

    vout.CenterW = vin.PosW;
    vout.SizeW = vin.SizeW;

    return vout;
}

[maxvertexcount(4)]
void GS(point VertexOut gin[1],
        uint primID : SV_PrimitiveID,
        inout TriangleStream<GeoOut> triStream){

    float3 up = float3(0.0f,1.0f,0.0f);
    float3 look = gEyePosW - gin[0].CenterW;
    look.y = 0.0f;
    look = normalize(look);
    float3 right = cross(up,look);

    float halfW = 0.5f * gin[0].SizeW.x;
    float halfH = 0.5f * gin[0].SizeW.y;

    float4 v[4];
    v[0] = float4(gin[0].CenterW + halfW * right - halfH*up, 1.0f);
    v[1] = float4(gin[0].CenterW + halfW * right + halfH*up, 1.0f);
    v[2] = float4(gin[0].CenterW - halfW * right - halfH*up, 1.0f);
    v[3] = float4(gin[0].CenterW - halfW * right + halfH*up, 1.0f);

    float2 texCoord[4] = 
    {
        float2(0.0f,1.0f),
        float2(0.0f,0.0f),
        float2(1.0f,1.0f),
        float2(1.0f,0.0f)
    };

    GeoOut gOut;
    [unroll]
    for(int i = 0; i < 4; i++)
    {
        gOut.PosH = mul(v[i], gViewProj);
        gOut.PosW = v[i].xyz;
        gOut.NormalW = look;
        gOut.TexC = texCoord[i];
        gOut.PrimID = primID;

        triStream.Append(gOut);
    }

}

float4 PS(GeoOut pin) : SV_Target
{
    MaterialData matData = gMaterialData[gMaterialIndex];

    float4 diffuseAlbedo = gTextureMaps[matData.DiffuseMapIndex].Sample(gsamAnisotropicWrap, pin.TexC) * matData.DiffuseAlbedo;

    clip(diffuseAlbedo.a - 0.1f);

    return diffuseAlbedo;


}
