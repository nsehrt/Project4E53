#include "TerrainCommon.hlsl"

#define SHADOW_ADD_BRIGHTNESS 0.4f

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
	
    // Transform to world space.
    float4 posW = mul(float4(vin.PosL, 1.0f), gWorld);
    vout.PosW = posW.xyz;

    // Assumes nonuniform scaling; otherwise, need to use inverse-transpose of world matrix.
    vout.NormalW = mul(vin.NormalL, (float3x3)gWorld);
	
	vout.TangentW = mul(vin.TangentU, (float3x3)gWorld);

    // Transform to homogeneous clip space.
    vout.PosH = mul(posW, gViewProj);
	
	// Output vertex attributes for interpolation across triangle.
	vout.TexC = mul(float4(vin.TexC, 0.0f, 1.0f), gTexTransform).xy;
	
    // Generate projective tex-coords to project shadow map onto scene.
    vout.ShadowPosH = mul(posW, gShadowTransform);

    vout.TexBlend = vin.TexBlend;

    return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
	// Fetch the material data.
	MaterialData matData = gMaterialData[gMaterialIndex];
	float4 diffuseAlbedo = matData.DiffuseAlbedo;
	float3 fresnelR0 = matData.FresnelR0;
	float  roughness = matData.Roughness;

	// Dynamically look up the texture in the array.
	float4 c0 = gBlendTexture1.Sample( gsamAnisotropicWrap, pin.TexC );
	float4 c1 = gBlendTexture2.Sample( gsamAnisotropicWrap, pin.TexC );
	float4 c2 = gBlendTexture3.Sample( gsamAnisotropicWrap, pin.TexC );
	float4 c3 = gBlendTexture4.Sample( gsamAnisotropicWrap, pin.TexC );

    diffuseAlbedo = lerp(diffuseAlbedo, c0, pin.TexBlend.x);
    diffuseAlbedo = lerp(diffuseAlbedo, c1, pin.TexBlend.y);    
    diffuseAlbedo = lerp(diffuseAlbedo, c2, pin.TexBlend.z);
    diffuseAlbedo = lerp(diffuseAlbedo, c3, pin.TexBlend.w);

	// Interpolating normal can unnormalize it, so renormalize it.
    pin.NormalW = normalize(pin.NormalW);
	
	// float4 normalMapSample = gTextureMaps[normalMapIndex].Sample(gsamAnisotropicWrap, pin.TexC);
	// float3 bumpedNormalW = NormalSampleToWorldSpace(normalMapSample.rgb, pin.NormalW, pin.TangentW);
    float3 bumpedNormalW = pin.NormalW;



    // Vector from point being lit to eye. 
    float3 toEyeW = normalize(gEyePosW - pin.PosW);

    // Light terms.
    float4 ambient = gAmbientLight*diffuseAlbedo;

    // Only the first light casts a shadow.
    float3 shadowFactor = float3(1.0f, 1.0f, 1.0f);
    
    shadowFactor[0] = saturate(CalcShadowFactor(pin.ShadowPosH) + SHADOW_ADD_BRIGHTNESS);

    const float shininess = (1.0f - roughness);
    Material mat = { diffuseAlbedo, fresnelR0, shininess };
    float4 directLight = ComputeLighting(gLights, mat, pin.PosW,
        bumpedNormalW, toEyeW, shadowFactor);

    float4 litColor = ambient + directLight;

	// Add in specular reflections.
	float3 r = reflect(-toEyeW, bumpedNormalW);
	float3 fresnelFactor = SchlickFresnel(fresnelR0, bumpedNormalW, r);
    litColor.rgb += shininess * fresnelFactor;
	
    // Common convention to take alpha from diffuse albedo.
    litColor.a = diffuseAlbedo.a;

    //return litColor.rrra; /*grey scale image*/
    return litColor;
}