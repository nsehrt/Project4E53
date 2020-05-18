#pragma once

#include "../util/d3dUtil.h"
#include "../util/mathhelper.h"
#include "../render/uploadbuffer.h"

struct ObjectConstants
{
    DirectX::XMFLOAT4X4 World = MathHelper::identity4x4();
    DirectX::XMFLOAT4X4 WorldInvTranspose = MathHelper::identity4x4();
    unsigned int MaterialIndex = 0;
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
    DirectX::XMFLOAT4X4 ShadowTransform = MathHelper::identity4x4();
    DirectX::XMFLOAT3 EyePosW = { 0.0f, 0.0f, 0.0f };
    float cbPerObjectPad1 = 0.0f;
    DirectX::XMFLOAT2 RenderTargetSize = { 0.0f, 0.0f };
    DirectX::XMFLOAT2 InvRenderTargetSize = { 0.0f, 0.0f };
    float NearZ = 0.0f;
    float FarZ = 0.0f;
    float TotalTime = 0.0f;
    float DeltaTime = 0.0f;

    DirectX::XMFLOAT4 AmbientLight = { 0.0f, 0.0f, 0.0f, 1.0f };

    /*3 directional*/
    /*4 point*/
    /*1 spot*/
    Light Lights[8];
};

struct MaterialData
{
    DirectX::XMFLOAT4 DiffuseAlbedo = { 1.0f, 1.0f, 1.0f, 1.0f };
    DirectX::XMFLOAT3 FresnelR0 = { 0.01f, 0.01f, 0.01f };
    float Roughness = 0.5f;

    // Used in texture mapping.
    DirectX::XMFLOAT4X4 MatTransform = MathHelper::identity4x4();
    DirectX::XMFLOAT4X4 DisplacementTransform0 = MathHelper::identity4x4();
    DirectX::XMFLOAT4X4 DisplacementTransform1 = MathHelper::identity4x4();
    DirectX::XMFLOAT4X4 NormalTransform0 = MathHelper::identity4x4();
    DirectX::XMFLOAT4X4 NormalTransform1 = MathHelper::identity4x4();

    UINT DiffuseMapIndex = 0;
    UINT NormalMapIndex = 0;
    UINT Displacement1Index = 0;
    UINT Displacement2Index = 0;
    UINT MiscTexture1Index = 0; 
    UINT MiscTexture2Index = 0;
    UINT pad1 = 0;
    UINT pad2 = 0;
};

struct Vertex
{
    DirectX::XMFLOAT3 Pos;
    DirectX::XMFLOAT3 Normal;
    DirectX::XMFLOAT2 TexC;
    DirectX::XMFLOAT3 TangentU;
};

struct TerrainVertex
{
    DirectX::XMFLOAT3 Pos;
    DirectX::XMFLOAT3 Normal;
    DirectX::XMFLOAT2 TexC;
    DirectX::XMFLOAT3 TangentU;
    DirectX::XMFLOAT4 TexBlend;
};

struct BillBoardVertex
{
    DirectX::XMFLOAT3 Pos;
    DirectX::XMFLOAT2 Size;
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

    FrameResource(ID3D12Device* device, UINT passCount, UINT objectCount, UINT skinnedObjectCount, UINT materialCount, UINT terrainVertexCount);
    FrameResource(const FrameResource& rhs) = delete;
    FrameResource& operator=(const FrameResource& rhs) = delete;
    ~FrameResource();

    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> CmdListAlloc;
    std::unique_ptr<UploadBuffer<PassConstants>> PassCB = nullptr;
    std::unique_ptr<UploadBuffer<ObjectConstants>> ObjectCB = nullptr;

    std::unique_ptr<UploadBuffer<MaterialData>> MaterialBuffer = nullptr;
    std::unique_ptr<UploadBuffer<TerrainVertex>> TerrainVB = nullptr;

    //std::unique_ptr<UploadBuffer<SkinnedConstants>> SkinnedCB = nullptr;
//std::unique_ptr<UploadBuffer<SsaoConstants>> SsaoCB = nullptr;

    // Fence value to mark commands up to this fence point.  This lets us
    // check if these frame resources are still in use by the GPU.
    UINT64 Fence = 0;
};