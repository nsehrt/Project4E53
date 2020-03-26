#pragma once

#include "../util/d3dUtil.h"
#include "../util/mathhelper.h"
#include "../render/uploadbuffer.h"

struct ObjectConstants
{
    DirectX::XMFLOAT4X4 World = MathHelper::identity4x4();
    DirectX::XMFLOAT4X4 TexTransform = MathHelper::identity4x4();
    unsigned int MaterialIndex = -1;
    unsigned int OnjPad0 = 0;
    unsigned int OnjPad1 = 0;
    unsigned int OnjPad2 = 0;
};

struct SkinnedConstants
{
    DirectX::XMFLOAT4X4 BoneTransforms[96];
};

struct PassConstants
{
    DirectX::XMFLOAT4X4 View = MathHelper::identity4x4();
    DirectX::XMFLOAT4X4 InvView = MathHelper::identity4x4();
    DirectX::XMFLOAT4X4 Proj = MathHelper::identity4x4();
    DirectX::XMFLOAT4X4 InvProj = MathHelper::identity4x4();
    DirectX::XMFLOAT4X4 ViewProj = MathHelper::identity4x4();
    DirectX::XMFLOAT4X4 InvViewProj = MathHelper::identity4x4();
    DirectX::XMFLOAT3 EyePosW = { 0.0f, 0.0f, 0.0f };
    float cbPerObjectPad1 = 0.0f;
    DirectX::XMFLOAT2 RenderTargetSize = { 0.0f, 0.0f };
    DirectX::XMFLOAT2 InvRenderTargetSize = { 0.0f, 0.0f };
    float NearZ = 0.0f;
    float FarZ = 0.0f;
    float TotalTime = 0.0f;
    float DeltaTime = 0.0f;

    DirectX::XMFLOAT4 AmbientLight = { 0.0f, 0.0f, 0.0f, 1.0f };

    // Indices [0, NUM_DIR_LIGHTS) are directional lights;
    // indices [NUM_DIR_LIGHTS, NUM_DIR_LIGHTS+NUM_POINT_LIGHTS) are point lights;
    // indices [NUM_DIR_LIGHTS+NUM_POINT_LIGHTS, NUM_DIR_LIGHTS+NUM_POINT_LIGHT+NUM_SPOT_LIGHTS)
    // are spot lights for a maximum of MaxLights per object.
    Light Lights[16];
};

struct MaterialData
{
    DirectX::XMFLOAT4 DiffuseAlbedo = { 1.0f, 1.0f, 1.0f, 1.0f };
    DirectX::XMFLOAT3 FresnelR0 = { 0.01f, 0.01f, 0.01f };
    float Roughness = 0.5f;

    // Used in texture mapping.
    DirectX::XMFLOAT4X4 MatTransform = MathHelper::identity4x4();

    UINT DiffuseMapIndex = 0;
    UINT NormalMapIndex = 0;
    UINT MaterialPad1 = 0;
    UINT MaterialPad2 = 0;
};

struct Vertex
{
    DirectX::XMFLOAT3 Pos;
    DirectX::XMFLOAT3 Normal;
    DirectX::XMFLOAT2 TexC;
    DirectX::XMFLOAT3 TangentU;
};

struct SkinnedVertex
{
    DirectX::XMFLOAT3 Pos;
    DirectX::XMFLOAT3 Normal;
    DirectX::XMFLOAT2 TexC;
    DirectX::XMFLOAT3 TangentU;
    DirectX::XMFLOAT3 BoneWeights;
    BYTE BoneIndices[4];
};

class FrameResource
{
public:

    FrameResource(ID3D12Device* device, UINT passCount, UINT objectCount, UINT skinnedObjectCount, UINT materialCount);
    FrameResource(const FrameResource& rhs) = delete;
    FrameResource& operator=(const FrameResource& rhs) = delete;
    ~FrameResource();

    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> CmdListAlloc;
    std::unique_ptr<UploadBuffer<PassConstants>> PassCB = nullptr;
    std::unique_ptr<UploadBuffer<ObjectConstants>> ObjectCB = nullptr;
    //std::unique_ptr<UploadBuffer<SkinnedConstants>> SkinnedCB = nullptr;
    //std::unique_ptr<UploadBuffer<SsaoConstants>> SsaoCB = nullptr;
    std::unique_ptr<UploadBuffer<MaterialData>> MaterialBuffer = nullptr;

    // Fence value to mark commands up to this fence point.  This lets us
    // check if these frame resources are still in use by the GPU.
    UINT64 Fence = 0;
};