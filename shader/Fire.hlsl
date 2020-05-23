#include "ParticleCommon.hlsl"

VertexOut VS(VertexIn vin, uint vertID: SV_VERTEXID){
    VertexOut vout;

    vout.CenterW = mul(float4(vin.PosL, 1.0f), gWorld).xyz;
    vout.CenterW += 0.5f * vin.Age * vin.Age * gMaterialData[gMaterialIndex].FresnelR0 + vin.Age * vin.Velocity; /*Acceleration stored in Fresnel of Material*/

	float opacity = 1.0f - smoothstep(0.0f, 1.0f, vin.Age/gMaterialData[gMaterialIndex].Roughness);
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
	uint diffuseMapIndex = matData.DiffuseMapIndex;
	uint normalMapIndex = matData.NormalMapIndex;

    float4 color = gTextureMaps[matData.DiffuseMapIndex].Sample(gsamLinearWrap, pin.TexC) * diffuseAlbedo * pin.Color;

    clip(color.a - 0.1f);

    return color;
}
