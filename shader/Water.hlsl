#include "Common.hlsl"

#define SHADOW_ADD_BRIGHTNESS 0.2f
struct VertexIn
{
	float3 PosL    : POSITION;
    float3 NormalL : NORMAL;
	float2 TexC    : TEXCOORD;
	float3 TangentU : TANGENT;
};

struct VertexOut
{
	float4 PosH    : SV_POSITION;
    float4 ShadowPosH : POSITION0;
    float3 PosW    : POSITION1;
    float3 NormalW : NORMAL;
	float3 TangentW : TANGENT;
	float2 TexC    : TEXCOORD;
};


VertexOut VS(VertexIn vin)
{
	VertexOut vout = (VertexOut)0.0f;

	// Fetch the material data.
	MaterialData matData = gMaterialData[gMaterialIndex];
	

    // Add displacement
    float2 disp =  mul(float4(vin.TexC, 0.0f, 1.0f), matData.Displacement1Transform).xy;
    float2 disp2 = mul(float4(vin.TexC, 0.0f,1.0f), matData.Displacement2Transform).xy;

    vin.PosL.y = gTextureMaps[matData.Displacement1Index].SampleLevel(gsamAnisotropicWrap, disp,0).y +
                 gTextureMaps[matData.Displacement2Index].SampleLevel(gsamAnisotropicWrap, disp2,0).y;

    // Transform to world space.
    float4 posW = mul(float4(vin.PosL, 1.0f), gWorld);
    vout.PosW = posW.xyz;

    // Assumes nonuniform scaling; otherwise, need to use inverse-transpose of world matrix.
    vout.NormalW = mul(vin.NormalL, (float3x3)gWorld);
	
	vout.TangentW = mul(vin.TangentU, (float3x3)gWorld);

    // Transform to homogeneous clip space.
    vout.PosH = mul(posW, gViewProj);
	
	// Output vertex attributes for interpolation across triangle.
	float4 texC = mul(float4(vin.TexC, 0.0f, 1.0f), gTexTransform);
	vout.TexC = mul(texC, matData.MatTransform).xy;

    // Generate projective tex-coords to project shadow map onto scene.
    vout.ShadowPosH = mul(posW, gShadowTransform);

    return vout;
}


float4 PS(VertexOut pin) : SV_Target
{
	// Fetch the material data.
	MaterialData matData = gMaterialData[gMaterialIndex];
	float4 diffuseAlbedo = matData.DiffuseAlbedo;
	float3 fresnelR0 = matData.FresnelR0;
	float  roughness = matData.Roughness;
	uint diffuseMapIndex = matData.DiffuseMapIndex;
	uint normalMapIndex = matData.NormalMapIndex;

	// Dynamically look up the texture in the array.
	diffuseAlbedo *= gTextureMaps[diffuseMapIndex].Sample(gsamAnisotropicWrap, pin.TexC);
    
	// Interpolating normal can unnormalize it, so renormalize it.
    pin.NormalW = normalize(pin.NormalW);
	
	float4 normalMapSample = gTextureMaps[normalMapIndex].Sample(gsamAnisotropicWrap, pin.TexC);
	float3 bumpedNormalW = NormalSampleToWorldSpace(normalMapSample.rgb, pin.NormalW, pin.TangentW);


    float4 normalMapSample0 = gTextureMaps[matData.Displacement1NormalIndex].Sample(gsamAnisotropicWrap, pin.TexC);
    float3 bumpedNormalW0 = NormalSampleToWorldSpace(normalMapSample0.rgb, pin.NormalW, pin.TangentW);

    float4 normalMapSample1 = gTextureMaps[matData.Displacement2NormalIndex].Sample(gsamAnisotropicWrap, pin.TexC);
    float3 bumpedNormalW1 = NormalSampleToWorldSpace(normalMapSample1.rgb, pin.NormalW, pin.TangentW);

    bumpedNormalW = normalize(bumpedNormalW0 + bumpedNormalW1);

    // Vector from point being lit to eye. 
    float3 toEyeW = normalize(gEyePosW - pin.PosW);

    // Light terms.
    float4 ambient = gAmbientLight*diffuseAlbedo;

    // Only the first light casts a shadow.
    float3 shadowFactor = float3(1.0f, 1.0f, 1.0f);
    
    shadowFactor[0] = saturate(CalcShadowFactor(pin.ShadowPosH) + SHADOW_ADD_BRIGHTNESS);

    const float shininess = (1.0f - roughness) * normalMapSample.a;
    Material mat = { diffuseAlbedo, fresnelR0, shininess };
    float4 directLight = ComputeLighting(gLights, mat, pin.PosW,
        bumpedNormalW, toEyeW, shadowFactor);

    float4 litColor = ambient + directLight;

	// Add in specular reflections.
	float3 r = reflect(-toEyeW, bumpedNormalW);
	float4 reflectionColor = gCubeMap.Sample(gsamLinearWrap, r);
	float3 fresnelFactor = SchlickFresnel(fresnelR0, bumpedNormalW, r);
    litColor.rgb += shininess * fresnelFactor * reflectionColor.rgb;
	
    // // Common convention to take alpha from diffuse albedo.
    litColor.a = diffuseAlbedo.a; 
    

    // //return litColor.rrra; /*grey scale image*/
    return litColor;
}