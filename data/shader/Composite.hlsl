/*Multiply two input textures and add some post process fx*/

#define INC_BRIGHTNESS
#define DIGITAL_VIBRANCE

Texture2D gBaseMap : register(t0);
Texture2D gEdgeMap : register(t1);

SamplerState gsamPointWrap        : register(s0);
SamplerState gsamPointClamp       : register(s1);
SamplerState gsamLinearWrap       : register(s2);
SamplerState gsamLinearClamp      : register(s3);
SamplerState gsamAnisotropicWrap  : register(s4);
SamplerState gsamAnisotropicClamp : register(s5);
SamplerComparisonState gsamShadow : register(s6);

static const float2 gTexCoords[6] = 
{
	float2(0.0f, 1.0f),
	float2(0.0f, 0.0f),
	float2(1.0f, 0.0f),
	float2(0.0f, 1.0f),
	float2(1.0f, 0.0f),
	float2(1.0f, 1.0f)
};

static float4 coefLuma = {0.212656f, 0.714158f, 0.072186f, 1.0f};
static float4 rgbBalance = {1.0f,1.0f,1.0f, 1.0f};
static float vibrance = 0.13f;

struct VertexOut
{
	float4 PosH    : SV_POSITION;
	float2 TexC    : TEXCOORD;
};

VertexOut VS(uint vid : SV_VertexID)
{
	VertexOut vout;
	
	vout.TexC = gTexCoords[vid];
	vout.PosH = float4(2.0f*vout.TexC.x - 1.0f, 1.0f - 2.0f*vout.TexC.y, 0.0f, 1.0f);

    return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
    float4 c = gBaseMap.SampleLevel(gsamPointClamp, pin.TexC, 1.0f);
	float4 e = gEdgeMap.SampleLevel(gsamPointClamp, pin.TexC, 1.0f);
	
	float4 outColor = c*e;

	/*apply digital vibrance*/
	#ifdef DIGITAL_VIBRANCE

	float4 luma = dot(coefLuma, outColor);
	float4 maxColor = max(outColor.r, max(outColor.g, outColor.b));
	float4 minColor = min(outColor.r, min(outColor.g, outColor.b));

	float4 colorSat = maxColor - minColor;
	float4 coeffVibrance = float4(rgbBalance * vibrance);

	outColor = lerp(luma, outColor, 1.0f + (coeffVibrance * (1.0f - (sign(coeffVibrance)* colorSat))));

	#endif

	/*increase brightness*/
	#ifdef INC_BRIGHTNESS
	outColor = saturate(outColor*1.4f);
	#endif

	return outColor;
}


