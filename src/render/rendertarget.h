#pragma once

#include "../util/d3dUtil.h"
#include <DirectXColors.h>

class RenderTarget
{
public:
    explicit RenderTarget(ID3D12Device* device,
                          UINT width,
                          UINT height,
                          DXGI_FORMAT format);

    RenderTarget(const RenderTarget& rhs) = delete;
    RenderTarget& operator=(const RenderTarget& rhs) = delete;
    ~RenderTarget() = default;

    ID3D12Resource* getResource();
    CD3DX12_GPU_DESCRIPTOR_HANDLE getSrv();
    CD3DX12_CPU_DESCRIPTOR_HANDLE getRtv();

    void buildDescriptors(CD3DX12_CPU_DESCRIPTOR_HANDLE cpuSrvHandle,
                          CD3DX12_GPU_DESCRIPTOR_HANDLE gpuSrvHandle,
                          CD3DX12_CPU_DESCRIPTOR_HANDLE cpuRtvHandle);

    void onResize(UINT _width, UINT _height);

private:
    void buildDescriptors();
    void buildResource();

    ID3D12Device* mDevice = nullptr;

    UINT mWidth = 0;
    UINT mHeight = 0;
    DXGI_FORMAT mFormat = DXGI_FORMAT_R8G8B8A8_UNORM;

    CD3DX12_CPU_DESCRIPTOR_HANDLE mCpuSrvHandle;
    CD3DX12_CPU_DESCRIPTOR_HANDLE mCpuRtvHandle;
    CD3DX12_GPU_DESCRIPTOR_HANDLE mGpuSrvHandle;

    Microsoft::WRL::ComPtr<ID3D12Resource> mOffscreenTexture = nullptr;
};