#include "DefaultCommon.hlsl"

struct VertexIn
{
	float3 PosL    : POSITION;
    float3 NormalL : NORMAL;
	float2 Tex    : TEXCOORD;
	float3 TangentL : TANGENT;
};

struct VertexOut
{
    float3 PosW           : POSITION0;
	float4 ShadowPosH : POSITION1;
    float3 NormalW        : NORMAL;
	float3 TangentW       : TANGENT;
	float2 Tex           : TEXCOORD0;
	float2 WaveDispTex0   : TEXCOORD1;
	float2 WaveDispTex1   : TEXCOORD2;
	float2 WaveNormalTex0 : TEXCOORD3;
	float2 WaveNormalTex1 : TEXCOORD4;
	float  TessFactor     : TESS;
};

static const float gMaxTessDistance = 4.0f;
static const float gMinTessDistance = 30.0f;
static const float gMinTessFactor = 2.0f;
static const float gMaxTessFactor = 6.0f;


VertexOut VS(VertexIn vin)
{
	VertexOut vout;
	
    MaterialData matData = gMaterialData[gMaterialIndex];

	// Transform to world space space.
	vout.PosW     = mul(float4(vin.PosL, 1.0f), gWorld).xyz;
	vout.NormalW  = mul(vin.NormalL, (float3x3) gWorldInvTranspose);
	vout.TangentW = mul(vin.TangentL, (float3x3)gWorld);

	// Output vertex attributes for interpolation across triangle.
	vout.Tex            = mul(float4(vin.Tex, 0.0f, 1.0f), matData.MatTransform).xy;
	vout.WaveDispTex0   = mul(float4(vin.Tex, 0.0f, 1.0f), matData.Displacement1Transform).xy;
	vout.WaveDispTex1   = mul(float4(vin.Tex, 0.0f, 1.0f), matData.Displacement2Transform).xy;

	vout.WaveNormalTex0 = mul(float4(vin.Tex, 0.0f, 1.0f), matData.Normal1Transform).xy;
	vout.WaveNormalTex1 = mul(float4(vin.Tex, 0.0f, 1.0f), matData.Normal2Transform).xy;

	vout.ShadowPosH = mul(float4(vout.PosW,1.0f), gShadowTransform);

	float d = distance(vout.PosW, gEyePosW);

	float tess = saturate( (gMinTessDistance - d) / (gMinTessDistance - gMaxTessDistance));
	vout.TessFactor = gMinTessFactor + tess*(gMaxTessFactor-gMinTessFactor);

	return vout;
}


struct PatchTess
{
	float EdgeTess[3] : SV_TessFactor;
	float InsideTess  : SV_InsideTessFactor;
};


PatchTess ConstantHS(InputPatch<VertexOut,3> patch, 
                  uint patchID : SV_PrimitiveID)
{
	PatchTess pt;
	
	pt.EdgeTess[0] = 0.5f*(patch[1].TessFactor + patch[2].TessFactor);
	pt.EdgeTess[1] = 0.5f*(patch[2].TessFactor + patch[0].TessFactor);
	pt.EdgeTess[2] = 0.5f*(patch[0].TessFactor + patch[1].TessFactor);
	pt.InsideTess  = pt.EdgeTess[0];
	
	return pt;
}


struct HullOut
{
	float3 PosW     : POSITION0;
	float4 ShadowPosH : POSITION1;
    float3 NormalW  : NORMAL;
	float3 TangentW : TANGENT;
	float2 Tex            : TEXCOORD0;
	float2 WaveDispTex0   : TEXCOORD1;
	float2 WaveDispTex1   : TEXCOORD2;
	float2 WaveNormalTex0 : TEXCOORD3;
	float2 WaveNormalTex1 : TEXCOORD4;
};


[domain("tri")]
[partitioning("fractional_odd")]
[outputtopology("triangle_cw")]
[outputcontrolpoints(3)]
[patchconstantfunc("ConstantHS")]
HullOut HS(InputPatch<VertexOut,3> p, 
           uint i : SV_OutputControlPointID,
           uint patchId : SV_PrimitiveID)
{
	HullOut hout;
	
	// Pass through shader.
	hout.PosW           = p[i].PosW;
	hout.ShadowPosH 	= p[i].ShadowPosH;
	hout.NormalW        = p[i].NormalW;
	hout.TangentW       = p[i].TangentW;
	hout.Tex            = p[i].Tex;
	hout.WaveDispTex0   = p[i].WaveDispTex0;
	hout.WaveDispTex1   = p[i].WaveDispTex1;
	hout.WaveNormalTex0 = p[i].WaveNormalTex0;
	hout.WaveNormalTex1 = p[i].WaveNormalTex1;

	return hout;
}

struct DomainOut
{
	float4 PosH     : SV_POSITION;
    float3 PosW     : POSITION0;
	float4 ShadowPosH : POSITION1;
    float3 NormalW  : NORMAL;
	float3 TangentW : TANGENT;
	float2 Tex            : TEXCOORD0;
	float2 WaveDispTex0   : TEXCOORD1;
	float2 WaveDispTex1   : TEXCOORD2;
	float2 WaveNormalTex0 : TEXCOORD3;
	float2 WaveNormalTex1 : TEXCOORD4;
	
};

[domain("tri")]
DomainOut DS(PatchTess patchTess, 
             float3 bary : SV_DomainLocation, 
             const OutputPatch<HullOut,3> tri)
{
	DomainOut dout;
	MaterialData matData = gMaterialData[gMaterialIndex];

	// Interpolate patch attributes to generated vertices.
	dout.PosW           = bary.x*tri[0].PosW           + bary.y*tri[1].PosW           + bary.z*tri[2].PosW;
	dout.ShadowPosH     = bary.x*tri[0].ShadowPosH           + bary.y*tri[1].ShadowPosH           + bary.z*tri[2].ShadowPosH;
	dout.NormalW        = bary.x*tri[0].NormalW        + bary.y*tri[1].NormalW        + bary.z*tri[2].NormalW;
	dout.TangentW       = bary.x*tri[0].TangentW       + bary.y*tri[1].TangentW       + bary.z*tri[2].TangentW;
	dout.Tex            = bary.x*tri[0].Tex            + bary.y*tri[1].Tex            + bary.z*tri[2].Tex;
	dout.WaveDispTex0   = bary.x*tri[0].WaveDispTex0   + bary.y*tri[1].WaveDispTex0   + bary.z*tri[2].WaveDispTex0;
	dout.WaveDispTex1   = bary.x*tri[0].WaveDispTex1   + bary.y*tri[1].WaveDispTex1   + bary.z*tri[2].WaveDispTex1;
	dout.WaveNormalTex0 = bary.x*tri[0].WaveNormalTex0 + bary.y*tri[1].WaveNormalTex0 + bary.z*tri[2].WaveNormalTex0;
	dout.WaveNormalTex1 = bary.x*tri[0].WaveNormalTex1 + bary.y*tri[1].WaveNormalTex1 + bary.z*tri[2].WaveNormalTex1;

	// Interpolating normal can unnormalize it, so normalize it.
	dout.NormalW = normalize(dout.NormalW);
	
	//
	// Displacement mapping.
	//
	
	// Choose the mipmap level based on distance to the eye; specifically, choose
	// the next miplevel every MipInterval units, and clamp the miplevel in [0,6].
	const float MipInterval = 20.0f;
	float mipLevel = clamp( (distance(dout.PosW, gEyePosW) - MipInterval) / MipInterval, 0.0f, 6.0f);
	
	// Sample height map (stored in alpha channel).
	float h0 = gTextureMaps[matData.Displacement1Index].SampleLevel(gsamLinearWrap, dout.WaveDispTex0, mipLevel).a;
	float h1 = gTextureMaps[matData.Displacement2Index].SampleLevel(gsamLinearWrap, dout.WaveDispTex1, mipLevel).a;

	dout.PosW.y += matData.MiscFloat1*h0;
	dout.PosW.y += matData.MiscFloat2*h1;

	// Project to homogeneous clip space.
	dout.PosH = mul(float4(dout.PosW, 1.0f), gViewProj);
	
	return dout;
}



float4 PS(DomainOut pin) : SV_Target
{
	// Fetch the material data.
	MaterialData matData = gMaterialData[gMaterialIndex];
	float4 diffuseAlbedo = matData.DiffuseAlbedo;
	float3 fresnelR0 = matData.FresnelR0;
	float  roughness = matData.Roughness;
	uint diffuseMapIndex = matData.DiffuseMapIndex;
    
	// Interpolating normal can unnormalize it, so renormalize it.
    pin.NormalW = normalize(pin.NormalW);

    // Vector from point being lit to eye. 
    float3 toEyeW = normalize(gEyePosW - pin.PosW);

	diffuseAlbedo *= gTextureMaps[diffuseMapIndex].Sample(gsamAnisotropicWrap, pin.Tex);

	float3 normalMapSample0 = gTextureMaps[matData.Displacement1Index].Sample(gsamLinearWrap, pin.WaveNormalTex0).rgb;
	float3 bumpedNormalW0 = NormalSampleToWorldSpace(normalMapSample0, pin.NormalW, pin.TangentW);

	float3 normalMapSample1 = gTextureMaps[matData.Displacement2Index].Sample(gsamLinearWrap, pin.WaveNormalTex1).rgb;
	float3 bumpedNormalW1 = NormalSampleToWorldSpace(normalMapSample1, pin.NormalW, pin.TangentW);
	 
	float3 bumpedNormalW = normalize(bumpedNormalW0 + bumpedNormalW1);

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
	
	float3 r = reflect(-toEyeW, bumpedNormalW);
	float4 reflectionColor = gCubeMap.Sample(gsamLinearWrap, r);
	float3 fresnelFactor = SchlickFresnel(fresnelR0, bumpedNormalW, r);
    litColor.rgb += shininess * fresnelFactor * reflectionColor.rgb;

    // // Common convention to take alpha from diffuse albedo.
    litColor.a = diffuseAlbedo.a; 
    

    // //return litColor.rrra; /*grey scale image*/
    return litColor;
}