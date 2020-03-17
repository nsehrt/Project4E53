#pragma once

#include <Windows.h>
#include <wrl.h>
#include <dxgi1_6.h>
#include <d3d12.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <DirectXPackedVector.h>
#include <DirectXCollision.h>
#include <string>
#include <unordered_map>
#include "../util/mathhelper.h"

/*structs for materials, textures etc.*/

extern int gNumFrameResources;

struct SubMesh
{
    UINT IndexCount = 0;
    UINT StartIndexLocation = 0;
    INT BaseVertexLocation = 0;

    DirectX::BoundingBox Bounds;
};

struct Mesh
{
    std::string name;


    Microsoft::WRL::ComPtr<ID3DBlob> VertexBufferCPU = nullptr;
    Microsoft::WRL::ComPtr<ID3DBlob> IndexBufferCPU = nullptr;

    Microsoft::WRL::ComPtr<ID3D12Resource> VertexBufferGPU = nullptr;
    Microsoft::WRL::ComPtr<ID3D12Resource> IndexBufferGPU = nullptr;

    Microsoft::WRL::ComPtr<ID3D12Resource> VertexBufferUploader = nullptr;
    Microsoft::WRL::ComPtr<ID3D12Resource> IndexBufferUploader = nullptr;

    UINT VertexByteStride = 0;
    UINT VertexBufferByteSize = 0;
    DXGI_FORMAT IndexFormat = DXGI_FORMAT_R16_UINT;
    UINT IndexBufferByteSize = 0;

    std::unordered_map<std::string, SubMesh> DrawArgs;

    D3D12_VERTEX_BUFFER_VIEW getVertexBufferView()const
    {
        D3D12_VERTEX_BUFFER_VIEW vbv;
        vbv.BufferLocation = VertexBufferGPU->GetGPUVirtualAddress();
        vbv.SizeInBytes = VertexBufferByteSize;
        vbv.StrideInBytes = VertexByteStride;
        return vbv;
    }

    D3D12_INDEX_BUFFER_VIEW getIndexBufferView() const
    {
        D3D12_INDEX_BUFFER_VIEW ibv;
        ibv.BufferLocation = IndexBufferGPU->GetGPUVirtualAddress();
        ibv.SizeInBytes = IndexBufferByteSize;
        ibv.Format = IndexFormat;
        return ibv;
    }

    void FreeUploaders()
    {
        VertexBufferUploader = nullptr;
        IndexBufferUploader = nullptr;
    }


};


struct Light
{
    DirectX::XMFLOAT3 Strength = { 0.5f,0.5f,0.5f };
    float FallOffStart = 1.0f;
    DirectX::XMFLOAT3 Direction = { 0.0f,-1.0f,0.0f };
    float FallOffEnd = 10.0f;
    DirectX::XMFLOAT3 Position = { 0.0f,0.0f,0.0f };
    float SpotPower = 128.0f;
};

struct MaterialConstants
{
    DirectX::XMFLOAT4 DiffuseAlbedo = { 1.0f,1.0f,1.0f,1.0f };
    DirectX::XMFLOAT3 FresnelR0 = { 0.01f,0.01f,0.01f };
    float Roughness = 0.2f;

    DirectX::XMFLOAT4X4 MaterialTransform = MathHelper::identity4x4();
};

struct Material
{
    std::string Name;

    int MaterialCBIndex = -1;

    int DiffuseSrvHeapIndex = -1;

    int NormalSrvHeapIndex = -1;

    int NumFramesDirty = gNumFrameResources;

    DirectX::XMFLOAT4 DiffuseAlbedo = { 1.0f,1.0f,1.0f,1.0f };
    DirectX::XMFLOAT3 FresnelR0 = { 0.01f,0.01f,0.01f };
    float Roughness = 0.2f;

    DirectX::XMFLOAT4X4 MaterialTransform = MathHelper::identity4x4();
};

enum class TextureType
{
    Texture2D,
    TextureCube
};

struct Texture
{
    std::string Name;
    std::wstring Filename;
    Microsoft::WRL::ComPtr<ID3D12Resource> Resource = nullptr;
    Microsoft::WRL::ComPtr<ID3D12Resource> UploadHeap = nullptr;
    TextureType Type = TextureType::Texture2D;
};