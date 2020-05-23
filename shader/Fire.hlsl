#include "ParticleCommon.hlsl"

static const float3 gAccelW = {0.0f, 7.8f, 0.0f};

VertexOut VS(VertexIn vin, uint vertID: SV_VERTEXID){
    VertexOut vout;

    vout.CenterW = mul(float4(vin.PosL, 1.0f), gWorld).xyz;
    vout.CenterW += 0.5f * vin.Age * vin.Age * gAccelW + vin.Age * vin.Velocity;

	float opacity = 1.0f - smoothstep(0.0f, 1.0f, vin.Age/1.0f);
	vout.Color = float4(1.0f, 1.0f, 1.0f, opacity);

    vout.SizeW = vin.SizeW;
    vout.Age = vin.Age;
    vout.Visible = vin.Visible;

    return vout;
}

[maxvertexcount(4)]
void GS(point VertexOut gin[1],
        uint primID : SV_PrimitiveID,
        inout TriangleStream<GeoOut> triStream){

    if(!gin[0].Visible){
        return;
    }
    
    /*billboard*/
    float3 look  = normalize(gEyePosW.xyz - gin[0].CenterW);
    float3 right = normalize(cross(float3(0,1,0), look));
    float3 up    = cross(look, right);

    float halfW = 0.5f * gin[0].SizeW.x;
    float halfH = 0.5f * gin[0].SizeW.y;

    float4 v[4];
    v[0] = float4(gin[0].CenterW + halfW * right - halfH*up, 1.0f);
    v[1] = float4(gin[0].CenterW + halfW * right + halfH*up, 1.0f);
    v[2] = float4(gin[0].CenterW - halfW * right - halfH*up, 1.0f);
    v[3] = float4(gin[0].CenterW - halfW * right + halfH*up, 1.0f);

    GeoOut gOut;
    [unroll]
    for(int i = 0; i < 4; i++)
    {
        gOut.PosH = mul(v[i], gViewProj);
        gOut.PosW = v[i].xyz;
        gOut.Color = gin[0].Color;
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

    return diffuseAlbedo * pin.Color;

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
