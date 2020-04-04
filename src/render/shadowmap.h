#pragma once

#include "../util/d3dUtil.h"

class ShadowMap
{

public:
    explicit ShadowMap(ID3D12Device* _device, UINT _width, UINT _height);
    ShadowMap(const ShadowMap& rhs) = delete;
    ShadowMap& operator=(const ShadowMap& rhs) = delete;
    ~ShadowMap() = default;

    ID3D12Resource* getResource() const
    {
        return mShadowMap.Get();
    }

    UINT getWidth()const
    {
        return width;
    }

    UINT getHeight()const
    {
        return height;
    }

    CD3DX12_GPU_DESCRIPTOR_HANDLE getSrv()const
    {
        return mSrvHandleGpu;
    }

    CD3DX12_CPU_DESCRIPTOR_HANDLE getDsv()const
    {
        return mDsvHandleCpu;
    }

    D3D12_VIEWPORT getViewPort()const
    {
        return mViewPort;
    }

    D3D12_RECT getScissor()const
    {
        return mScissor;
    }

    void buildDescriptors(CD3DX12_CPU_DESCRIPTOR_HANDLE _cpuSrv,
                         CD3DX12_GPU_DESCRIPTOR_HANDLE _gpuSrv,
                         CD3DX12_CPU_DESCRIPTOR_HANDLE _cpuDsv);

    void onResize(UINT _width, UINT _height);

    void updateShadowTransform();

    DirectX::BoundingSphere sceneBounds;

    UINT heapIndex = 0;

    /*shadow transform*/
    float mLightNearZ = 0.0f;
    float mLightFarZ = 0.0f;
    DirectX::XMFLOAT3 mLightPosW;
    DirectX::XMFLOAT4X4 mLightView = MathHelper::identity4x4();
    DirectX::XMFLOAT4X4 mLightProj = MathHelper::identity4x4();
    DirectX::XMFLOAT4X4 mShadowTransform = MathHelper::identity4x4();

    /*TODO*/
    float mLightRotationAngle = 0.0f;
    DirectX::XMFLOAT3 mBaseLightDirections[3] = {
         DirectX::XMFLOAT3(0.57735f, -0.57735f, 0.57735f),
         DirectX::XMFLOAT3(-0.57735f, -0.57735f, 0.57735f),
         DirectX::XMFLOAT3(0.0f, -0.707f, -0.707f)
    };
    DirectX::XMFLOAT3 mRotatedLightDirections[3];


private:

    ID3D12Device* mDevice = nullptr;
    D3D12_VIEWPORT mViewPort;
    D3D12_RECT mScissor;

    UINT width = 0;
    UINT height = 0;
    DXGI_FORMAT mFormat = DXGI_FORMAT_R24G8_TYPELESS;

    CD3DX12_CPU_DESCRIPTOR_HANDLE mSrvHandleCpu;
    CD3DX12_GPU_DESCRIPTOR_HANDLE mSrvHandleGpu;
    CD3DX12_CPU_DESCRIPTOR_HANDLE mDsvHandleCpu;

    Microsoft::WRL::ComPtr<ID3D12Resource> mShadowMap = nullptr;


    void buildDescriptors();
    void buildResource();
};