#include "TerrainCommon.hlsl"

struct VertexIn
{
	float3 PosL    : POSITION;
    float3 NormalL : NORMAL;
	float2 TexC    : TEXCOORD;
	float3 TangentU : TANGENT;
    float4 TexBlend : TEXBLEND;
};

struct VertexOut
{
	float4 PosH    : SV_POSITION;
    float4 ShadowPosH : POSITION0;
    float3 PosW    : POSITION1;
    float3 NormalW : NORMAL;
	float3 TangentW : TANGENT;
	float2 TexC    : TEXCOORD;
    float4 TexBlend : TEXBLEND;
};

VertexOut VS(VertexIn vin)
{
	VertexOut vout = (VertexOut)0.0f;
	
    float4 posW = mul(float4(vin.PosL, 1.0f), gWorld);
    vout.PosW = posW.xyz;
    vout.NormalW = mul(vin.NormalL, (float3x3)gWorld);	
	vout.TangentW = mul(vin.TangentU, (float3x3)gWorld);

    vout.PosH = mul(posW, gViewProj);
	
	vout.TexC = mul(float4(vin.TexC, 0.0f, 1.0f), gWorldInvTranspose).xy; 
	
    vout.ShadowPosH = mul(posW, gShadowTransform);

    vout.TexBlend = vin.TexBlend;

    return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
	MaterialData matData = gMaterialData[gMaterialIndex];
	float4 diffuseAlbedo = matData.DiffuseAlbedo;
	float3 fresnelR0 = matData.FresnelR0;
	float  roughness = matData.Roughness;

	float4 c0 = gBlendTexture1.Sample( gsamAnisotropicWrap, pin.TexC );
	float4 c1 = gBlendTexture2.Sample( gsamAnisotropicWrap, pin.TexC );
	float4 c2 = gBlendTexture3.Sample( gsamAnisotropicWrap, pin.TexC );
	float4 c3 = gBlendTexture4.Sample( gsamAnisotropicWrap, pin.TexC );

    diffuseAlbedo = lerp(diffuseAlbedo, c0, pin.TexBlend.x);
    diffuseAlbedo = lerp(diffuseAlbedo, c1, pin.TexBlend.y);    
    diffuseAlbedo = lerp(diffuseAlbedo, c2, pin.TexBlend.z);
    diffuseAlbedo = lerp(diffuseAlbedo, c3, pin.TexBlend.w);

    pin.NormalW = normalize(pin.NormalW);
	
    float3 bumpedNormalW = pin.NormalW;
    float3 toEyeW = normalize(gEyePosW - pin.PosW);

    float4 ambient = gAmbientLight*diffuseAlbedo;
    float3 shadowFactor = float3(1.0f, 1.0f, 1.0f);
    
    shadowFactor[0] = saturate(CalcShadowFactor(pin.ShadowPosH) + SHADOW_ADD_BRIGHTNESS);

    const float shininess = (1.0f - roughness);
    Material mat = { diffuseAlbedo, fresnelR0, shininess };
    float4 directLight = ComputeLighting(gLights, mat, pin.PosW,
        bumpedNormalW, toEyeW, shadowFactor);

    float4 litColor = ambient + directLight;

	float3 r = reflect(-toEyeW, bumpedNormalW);
	float3 fresnelFactor = SchlickFresnel(fresnelR0, bumpedNormalW, r);
    litColor.rgb += shininess * fresnelFactor;
	
    litColor.a = diffuseAlbedo.a;

    return litColor;
}