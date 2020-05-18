#include "DefaultCommon.hlsl"

struct VertexIn
{
	float3 PosL  : POSITION;
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
    float3 PosW    : POSITION0;
    float3 NormalW : NORMAL;
    float2 TexC    : TEXCOORD;
    uint   PrimID  : SV_PrimitiveID;
};


VertexOut VS(VertexIn vin, uint vertID: SV_VERTEXID){
    VertexOut vout;

    vout.CenterW = mul(float4(vin.PosL, 1.0f), gWorld).xyz;
    vout.SizeW = vin.SizeW;

    return vout;
}

[maxvertexcount(4)]
void GS(point VertexOut gin[1],
        uint primID : SV_PrimitiveID,
        inout TriangleStream<GeoOut> triStream){

    /*billboard*/
    float3 up = float3(0.0f,1.0f,0.0f);
    float3 look = gEyePosW - gin[0].CenterW;
    look.y = 0.0f;
    look = normalize(look);
    float3 right = cross(up,look);

    /*fixed*/
    // float3 up = float3(0.0f,1.0f,0.0f);
    // float3 look = float3(0.0f,0.0f,0.5f);
    // float3 right = float3(1.0f,0.0f,0.0f);

    float halfW = 0.5f * gin[0].SizeW.x;
    float halfH = 0.5f * gin[0].SizeW.y;

    float4 v[4];
    v[0] = float4(gin[0].CenterW + halfW * right - halfH*up, 1.0f);
    v[1] = float4(gin[0].CenterW + halfW * right + halfH*up, 1.0f);
    v[2] = float4(gin[0].CenterW - halfW * right - halfH*up, 1.0f);
    v[3] = float4(gin[0].CenterW - halfW * right + halfH*up, 1.0f);

    /*grass sway*/
    float swayFactor = gMaterialData[gMaterialIndex].DiffuseAlbedo.a * sin(gTotalTime + primID);
    swayFactor -= swayFactor * 0.5f;
    v[1].x += swayFactor;
    v[3].x += swayFactor;
    v[1].z += swayFactor*0.7f;
    v[3].z += swayFactor*0.7f;

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
	float4 diffuseAlbedo = matData.DiffuseAlbedo;
	float3 fresnelR0 = matData.FresnelR0;
	float  roughness = matData.Roughness;
	uint diffuseMapIndex = matData.DiffuseMapIndex;
	uint normalMapIndex = matData.NormalMapIndex;

    diffuseAlbedo = gTextureMaps[matData.DiffuseMapIndex].Sample(gsamAnisotropicClamp, pin.TexC) * float4(diffuseAlbedo.xyz, 1.0f);

    clip(diffuseAlbedo.a - 0.1f);

    //return diffuseAlbedo;

    /*lighting*/
    float3 bumpedNormalW = pin.NormalW;

   // Vector from point being lit to eye. 
    float3 toEyeW = normalize(gEyePosW - pin.PosW);

    // Only the first light casts a shadow.
    float3 shadowFactor = float3(1.0f, 1.0f, 1.0f);

    const float shininess = (1.0f - roughness);
    Material mat = { diffuseAlbedo, fresnelR0, shininess };
    float4 directLight = ComputeLightingNoDirectional(gLights, mat, pin.PosW,
        bumpedNormalW, toEyeW, shadowFactor);

    float4 litColor = diffuseAlbedo + directLight;
	
    // Common convention to take alpha from diffuse albedo.
    litColor.a = diffuseAlbedo.a; 

    return litColor;
}
