#pragma once

#include <Windows.h>
#include <wrl.h>
#include <dxgi1_6.h>
#include <d3d12.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <DirectXPackedVector.h>
#include <DirectXColors.h>
#include <DirectXCollision.h>
#include <string>
#include <memory>
#include <algorithm>
#include <vector>
#include <array>
#include <unordered_map>
#include <fstream>
#include <sstream>
#include <locale>
#include <codecvt>
#include <chrono>
#include <random>
#include "../extern/d3dx12.h"
#include "../extern/DDSTextureLoader.h"
#include "../util/mathhelper.h"


#ifndef ThrowIfFailed
#define ThrowIfFailed(x)                                              \
{                                                                     \
    HRESULT hr__ = (x);                                               \
    std::wstring wfn = AnsiToWString(__FILE__);                       \
    if(FAILED(hr__)) { throw DxException(hr__, L#x, wfn, __LINE__); } \
}
#endif

#ifndef ReleaseCom
#define ReleaseCom(x) { if(x){ x->Release(); x = 0; } }
#endif

#ifndef Assert
#define ASSERT(expr) \
    if(expr) { }\
    else\
    {\
        std::wostringstream os_;\
        os_ << "Assertion failed: " << #expr << " in " << __FILEW__ << " at line: " << __LINE__ <<endl;\
        OutputDebugString( os_.str().c_str() );\
        DebugBreak();\
    }
#else 
#define ASSERT(expr) //nothing
#endif

extern int gNumFrameResources;

inline std::wstring AnsiToWString(const std::string& cstr)
{
    WCHAR buffer[512];
    MultiByteToWideChar(CP_ACP, 0, cstr.c_str(), -1, buffer, 512);
    return std::wstring(buffer);
}

class d3dUtil
{
public:

    static bool isKeyDown(int vKeyCode);

    static UINT CalcConstantBufferSize(UINT byteSize)
    {
        return (byteSize + 255) & ~255;
    }

    static Microsoft::WRL::ComPtr<ID3DBlob> LoadBinary(const std::wstring& fileName);

    static Microsoft::WRL::ComPtr<ID3D12Resource> CreateDefaultBuffer(
        ID3D12Device* device,
        ID3D12GraphicsCommandList* cmdList,
        const void* initData,
        UINT64 byteSize,
        Microsoft::WRL::ComPtr<ID3D12Resource>& iuploadBuffer
    );

    static Microsoft::WRL::ComPtr<ID3DBlob> CompileShader(
        const std::wstring& fileName,
        const D3D_SHADER_MACRO* defines,
        const std::string& entryPoint,
        const std::string& target
    );

};

class DxException
{
public:
    DxException() = default;
    DxException(HRESULT hr, const std::wstring& functionName, const std::wstring& fileName, int lineNumber);

    std::wstring toString()const;

    HRESULT ErrorCode = S_OK;
    std::wstring FunctionName;
    std::wstring FileName;
    int LineNumber = -1;

};


/*structs for materials, textures etc.*/


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
    DirectX::XMFLOAT3 Strength = { 0.5,0.5,0.5 };
    float FallOffStart = 1.0f;
    DirectX::XMFLOAT3 Direction = { 0.0,-1.0,0.0 };
    float FallOffEnd = 10.0f;
    DirectX::XMFLOAT3 Position = { 0.0,0.0,0.0 };
    float SpotPower = 128.0;
};

struct MaterialConstants
{
    DirectX::XMFLOAT4 DiffuseAlbedo = { 1.0,1.0,1.0,1.0 };
    DirectX::XMFLOAT3 FresnelR0 = { 0.01,0.01,0.01 };
    float Roughness = 0.2;

    DirectX::XMFLOAT4X4 MaterialTransform = MathHelper::identity4x4();
};

struct Material
{
    std::string Name;

    int MaterialCBIndex = -1;

    int DiffuseSrvHeapIndex = -1;

    int NormalSrvHeapIndex = -1;

    int NumFramesDirty = gNumFrameResources;

    DirectX::XMFLOAT4 DiffuseAlbedo = { 1.0,1.0,1.0,1.0 };
    DirectX::XMFLOAT3 FresnelR0 = { 0.01,0.01,0.01 };
    float Roughness = 0.2;

    DirectX::XMFLOAT4X4 MaterialTransform = MathHelper::identity4x4();
};


struct Texture
{
    std::string Name;
    std::wstring Filename;
    Microsoft::WRL::ComPtr<ID3D12Resource> Resource = nullptr;
    Microsoft::WRL::ComPtr<ID3D12Resource> UploadHeap = nullptr;
};