#pragma once

#include "../util/d3dUtil.h"

class Blur
{
public:
    explicit Blur(ID3D12Device* _device, UINT _width, UINT _height, DXGI_FORMAT _format)
    {
        mDevice = _device;
        width = _width;
        height = _height;
        mFormat = _format;

        buildResources();
    }

    Blur(const Blur& rhs) = delete;
    Blur& operator=(const Blur& rhs) = delete;
    ~Blur() = default;

    ID3D12Resource* getOutput()
    {
        return mBlurMap0.Get();
    }

    void buildDescriptors(CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuDescriptor,
                          CD3DX12_GPU_DESCRIPTOR_HANDLE hGpuDescriptor,
                          UINT descriptorSize);

    void onResize(UINT _width, UINT _height)
    {
        if (width != _width || height != _height)
        {
            width = _width;
            height = _height;

            buildResources();
            buildDescriptors();
        }
    }

    void execute(ID3D12GraphicsCommandList* cmdList,
                 ID3D12RootSignature* rootSig,
                 ID3D12PipelineState* horzBlurPSO,
                 ID3D12PipelineState* vertBlurPSO,
                 ID3D12Resource* input
                 );

    void setSigma(float s)
    {
        mSigma = s;
    }

    float getSigma() const
    {
        return mSigma;
    }

    void setIterations(UINT i)
    {
        blurIterations = i;
    }

    UINT getIterations() const
    {
        return blurIterations;
    }

private:

    std::vector<float> calculateWeights(float sigma);

    void buildDescriptors();
    void buildResources();


    const int MaxRadius = 5;

    ID3D12Device* mDevice = nullptr;

    UINT width = 0;
    UINT height = 0;
    float mSigma = 0.0f; // 2.5f
    UINT blurIterations = 1;
    DXGI_FORMAT mFormat = DXGI_FORMAT_R8G8B8A8_UNORM;


    CD3DX12_CPU_DESCRIPTOR_HANDLE mBlur0CpuSrv;
    CD3DX12_CPU_DESCRIPTOR_HANDLE mBlur0CpuUav;

    CD3DX12_CPU_DESCRIPTOR_HANDLE mBlur1CpuSrv;
    CD3DX12_CPU_DESCRIPTOR_HANDLE mBlur1CpuUav;

    CD3DX12_GPU_DESCRIPTOR_HANDLE mBlur0GpuSrv;
    CD3DX12_GPU_DESCRIPTOR_HANDLE mBlur0GpuUav;

    CD3DX12_GPU_DESCRIPTOR_HANDLE mBlur1GpuSrv;
    CD3DX12_GPU_DESCRIPTOR_HANDLE mBlur1GpuUav;

    Microsoft::WRL::ComPtr<ID3D12Resource> mBlurMap0 = nullptr;
    Microsoft::WRL::ComPtr<ID3D12Resource> mBlurMap1 = nullptr;

};