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
#include "../extern/d3dx12.h"
#include "../extern/DDSTextureLoader.h"


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

extern const int gNumFrameResources;

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