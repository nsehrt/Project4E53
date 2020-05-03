#pragma once

#include "../util/d3dUtil.h"

class Sobel
{
public:
    explicit Sobel(ID3D12Device* device,
                   UINT width,
                   UINT height,
                   DXGI_FORMAT format)
    {
        mDevice = device;
        mWidth = width;
        mHeight = height;
        mFormat = format;

        buildResource();
    };

    Sobel(const Sobel& rhs) = delete;
    Sobel& operator=(const Sobel& rhs) = delete;
    ~Sobel() = default;

    CD3DX12_GPU_DESCRIPTOR_HANDLE getOutputSrv()
    {
        return mGpuSrvHandle;
    }

    UINT getDescriptorCount()const
    {
        return 2;
    }

    void buildDescriptors(
        CD3DX12_CPU_DESCRIPTOR_HANDLE cpuDescHandle,
        CD3DX12_GPU_DESCRIPTOR_HANDLE gpuDescHandle,
        UINT descriptorSize
    );

    void onResize(UINT width, UINT height);

    void execute(
        ID3D12GraphicsCommandList* cmdList,
        ID3D12RootSignature* rootSignature,
        ID3D12PipelineState* pso,
        CD3DX12_GPU_DESCRIPTOR_HANDLE input);

private:

    void buildDescriptors();
    void buildResource();

    ID3D12Device* mDevice = nullptr;
    UINT mWidth = 0;
    UINT mHeight = 0;
    DXGI_FORMAT mFormat = DXGI_FORMAT_R8G8B8A8_UNORM;

    CD3DX12_CPU_DESCRIPTOR_HANDLE mCpuSrvHandle;
    CD3DX12_CPU_DESCRIPTOR_HANDLE mCpuUavHandle;

    CD3DX12_GPU_DESCRIPTOR_HANDLE mGpuSrvHandle;
    CD3DX12_GPU_DESCRIPTOR_HANDLE mGpuUavHandle;

    Microsoft::WRL::ComPtr<ID3D12Resource> mOutput = nullptr;
};