#include "rendertarget.h"

RenderTarget::RenderTarget(ID3D12Device* device, UINT width, UINT height, DXGI_FORMAT format)
{
    mDevice = device;

    mWidth = width;
    mHeight = height;
    mFormat = format;

    buildResource();
}

ID3D12Resource* RenderTarget::getResource()
{
    return mOffscreenTexture.Get();
}

CD3DX12_GPU_DESCRIPTOR_HANDLE RenderTarget::getSrv()
{
    return mGpuSrvHandle;
}

CD3DX12_CPU_DESCRIPTOR_HANDLE RenderTarget::getRtv()
{
    return mCpuRtvHandle;
}

void RenderTarget::buildDescriptors(CD3DX12_CPU_DESCRIPTOR_HANDLE cpuSrvHandle, CD3DX12_GPU_DESCRIPTOR_HANDLE gpuSrvHandle, CD3DX12_CPU_DESCRIPTOR_HANDLE cpuRtvHandle)
{
    mCpuSrvHandle = cpuSrvHandle;
    mGpuSrvHandle = gpuSrvHandle;
    mCpuRtvHandle = cpuRtvHandle;

    buildDescriptors();
}

void RenderTarget::buildDescriptors()
{
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};

    srvDesc.Format = mFormat;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

    mDevice->CreateShaderResourceView(mOffscreenTexture.Get(), &srvDesc, mCpuSrvHandle);
    mDevice->CreateRenderTargetView(mOffscreenTexture.Get(), nullptr, mCpuRtvHandle);
}

void RenderTarget::buildResource()
{
    D3D12_RESOURCE_DESC textureDesc;
    ZeroMemory(&textureDesc, sizeof(D3D12_RESOURCE_DESC));

    textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    textureDesc.Alignment = 0;
    textureDesc.Width = mWidth;
    textureDesc.Height = mHeight;
    textureDesc.DepthOrArraySize = 1;
    textureDesc.MipLevels = 1;
    textureDesc.Format = mFormat;
    textureDesc.SampleDesc.Count = 1;
    textureDesc.SampleDesc.Quality = 0;
    textureDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    textureDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

    D3D12_CLEAR_VALUE clearValue = {};
    clearValue.Format = mFormat;
    clearValue.Color[0] = 0.196078449f; /*Colors::LimeGreen*/
    clearValue.Color[1] = 0.803921640f;
    clearValue.Color[2] = 0.196078449f;
    clearValue.Color[3] = 1.000000000f;

    const auto heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

    ThrowIfFailed(mDevice->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &textureDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        &clearValue,
        IID_PPV_ARGS(&mOffscreenTexture)
    ));
}

void RenderTarget::onResize(UINT _width, UINT _height)
{
    if ((mWidth != _width) || (mHeight != _height))
    {
        mWidth = _width;
        mHeight = _height;

        buildResource();
        buildDescriptors();
    }
}