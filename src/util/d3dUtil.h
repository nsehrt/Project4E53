#pragma once


#include <memory>
#include <algorithm>
#include <vector>
#include <array>
#include <fstream>
#include <locale>
#include <codecvt>
#include <chrono>
#include <random>
#include "../extern/d3dx12.h"
#include "../extern/DDSTextureLoader.h"
#include "../render/renderstructs.h"

#ifndef ThrowIfFailed
#define ThrowIfFailed(x)                                              \
{                                                                     \
    HRESULT hr__ = (x);                                               \
    std::wstring wfn = d3dUtil::s2ws(__FILE__);                       \
    if(FAILED(hr__)) { throw DxException(hr__, L#x, wfn, __LINE__); } \
}
#endif

#ifndef ReleaseCom
#define ReleaseCom(x) { if(x){ x->Release(); x = 0; } }
#endif

#ifdef _DEBUG
#define ASSERT(expr) \
    if(expr) { }\
    else\
    {\
        std::wostringstream os_;\
        os_ << "Assertion failed: " << #expr << " in " << __FILEW__ << " at line: " << __LINE__ <<std::endl;\
        OutputDebugString( os_.str().c_str() );\
        DebugBreak();\
    }
#else
#define ASSERT(expr)
#endif

#define DBOUT( s )           \
{                            \
   std::wostringstream os_;    \
   os_ << s;                   \
   OutputDebugString( os_.str().c_str() );  \
}

extern int gNumFrameResources;

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

    static Microsoft::WRL::ComPtr<ID3DBlob> CompileShaderFromString(
        const std::string& str,
        const D3D_SHADER_MACRO* defines,
        const std::string& entryPoint,
        const std::string& target
    );

    static inline std::wstring s2ws(const std::string& str)
    {
        const std::ctype<wchar_t>& CType = std::use_facet<std::ctype<wchar_t> >(std::locale());
        std::vector<wchar_t> wideStringBuffer(str.length());
        CType.widen(str.data(), str.data() + str.length(), &wideStringBuffer[0]);
        return std::wstring(&wideStringBuffer[0], wideStringBuffer.size());
    }

    static inline std::string ws2s(const std::wstring& s)
    {
        int len;
        int slength = (int)s.length() + 1;
        len = WideCharToMultiByte(CP_ACP, 0, s.c_str(), slength, 0, 0, 0, 0);
        std::string r(len, '\0');
        WideCharToMultiByte(CP_ACP, 0, s.c_str(), slength, &r[0], len, 0, 0);
        return r;
    }

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