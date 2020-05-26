cbuffer cbBlurKernel : register(b0)
{
    int gRadius;

    float w0;
    float w1;
    float w2;
    float w3;
    float w4;
    float w5;
    float w6;
    float w7;
    float w8;
    float w9;
    float w10;
};

static const int gMaxRadius = 5;

Texture2D gInput : register(t0);
RWTexture2D<float4> gOutput : register(u0);

#define N 256
#define CACHE_SIZE (N + 2*gMaxRadius)
groupshared float4 gCache[CACHE_SIZE];

[numthreads(N,1,1)]
void HorizontalBlurCS(int3 groupThreadID : SV_GroupThreadID,
                      int3 dispatchThreadID : SV_DispatchThreadID)
{
    float weights[11] = {w0,w1,w2,w3,w4,w5,w6,w7,w8,w9,w10};

    if(groupThreadID.x < gRadius)
    {
        /*clamp min*/
        int x = max(dispatchThreadID.x - gRadius, 0);
        gCache[groupThreadID.x] = gInput[int2(x, dispatchThreadID.y)];
    }

    if(groupThreadID.x >= N-gRadius)
    {
        /*clamp max*/
        int x = min(dispatchThreadID.x + gRadius, gInput.Length.x-1);
        gCache[groupThreadID.x+2*gRadius] = gInput[int2(x, dispatchThreadID.y)];
    }

    gCache[groupThreadID.x+gRadius] = gInput[min(dispatchThreadID.xy, gInput.Length.xy-1)];

    GroupMemoryBarrierWithGroupSync();

    float4 blur = float4(0,0,0,0);

    for(int i = -gRadius; i <= gRadius; i++){
        int k = groupThreadID.x + gRadius + i;
        blur += weights[i+gRadius]*gCache[k];
    }

    gOutput[dispatchThreadID.xy] = blur;
}

[numthreads(1,N,1)]
void VerticalBlurCS(int3 groupThreadID : SV_GroupThreadID,
                    int3 dispatchThreadID : SV_DispatchThreadID)
{
    float weights[11] = {w0,w1,w2,w3,w4,w5,w6,w7,w8,w9,w10};

    if(groupThreadID.y < gRadius)
    {
        /*clamp min*/
        int y = max(dispatchThreadID.y - gRadius, 0);
        gCache[groupThreadID.y] = gInput[int2(dispatchThreadID.x, y)];
    }

    if(groupThreadID.y >= N-gRadius)
    {
        /*clamp max*/
        int y = min(dispatchThreadID.y + gRadius, gInput.Length.y-1);
        gCache[groupThreadID.y+2*gRadius] = gInput[int2(dispatchThreadID.x, y)];
    }

    gCache[groupThreadID.y+gRadius] = gInput[min(dispatchThreadID.xy, gInput.Length.xy-1)];

    GroupMemoryBarrierWithGroupSync();

    float4 blur = float4(0,0,0,0);

    for(int i = -gRadius; i <= gRadius; i++){
        int k = groupThreadID.y + gRadius + i;
        blur += weights[i+gRadius]*gCache[k];
    }

    gOutput[dispatchThreadID.xy] = blur;
}