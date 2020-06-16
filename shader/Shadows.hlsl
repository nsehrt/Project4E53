#include "DefaultCommon.hlsl"

struct VertexIn
{
	float3 PosL    : POSITION;
	float2 TexC    : TEXCOORD;
	#ifdef SKINNED
    float3 BoneWeights : WEIGHTS;
    uint4 BoneIndices  : BONEINDICES;
	#endif
};

struct VertexOut
{
	float4 PosH    : SV_POSITION;
	float2 TexC    : TEXCOORD;
};

VertexOut VS(VertexIn vin)
{
	VertexOut vout = (VertexOut)0.0f;

	MaterialData matData = gMaterialData[gMaterialIndex];
	
#ifdef SKINNED
    float weights[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    weights[0] = vin.BoneWeights.x;
    weights[1] = vin.BoneWeights.y;
    weights[2] = vin.BoneWeights.z;
    weights[3] = 1.0f - weights[0] - weights[1] - weights[2];

    float3 posL = float3(0.0f, 0.0f, 0.0f);
    for(int i = 0; i < 4; ++i)
    {
        // Assume no nonuniform scaling when transforming normals, so 
        // that we do not have to use the inverse-transpose.

        posL += weights[i] * mul(float4(vin.PosL, 1.0f), gBoneTransforms[vin.BoneIndices[i]]).xyz;
    }

    vin.PosL = posL;
#endif

    // Transform to world space.
    float4 posW = mul(float4(vin.PosL, 1.0f), gWorld);

    // Transform to homogeneous clip space.
    vout.PosH = mul(posW, gViewProj);
	
	// Output vertex attributes for interpolation across triangle.
	vout.TexC = mul(float4(vin.TexC, 0.0f, 1.0f), matData.MatTransform).xy;
	
    return vout;
}

/*only use this for the purpose of clipping, use null pixel shader for opaque objects*/
void PS(VertexOut pin) 
{
	// Fetch the material data.
	MaterialData matData = gMaterialData[gMaterialIndex];
	float4 diffuseAlbedo = matData.DiffuseAlbedo;
    uint diffuseMapIndex = matData.DiffuseMapIndex;
	
	diffuseAlbedo *= gTextureMaps[diffuseMapIndex].Sample(gsamAnisotropicWrap, pin.TexC);

    clip(diffuseAlbedo.a - 0.1f);
}


