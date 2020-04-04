#include "shadowmap.h"

using namespace DirectX;

ShadowMap::ShadowMap(ID3D12Device* _device, UINT _width, UINT _height)
{
    mDevice = _device;

    width = _width;
    height = _height;

    mViewPort = { 0.0f,0.0f,(float)width, (float)height, 0.0,1.0f };
    mScissor = { 0,0,(int)width, (int)height };

    buildResource();

    sceneBounds.Center = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);
    sceneBounds.Radius = sqrtf(10.0f * 10.0f + 15.0f * 15.0f);
}

void ShadowMap::buildDescriptors(CD3DX12_CPU_DESCRIPTOR_HANDLE _cpuSrv, CD3DX12_GPU_DESCRIPTOR_HANDLE _gpuSrv, CD3DX12_CPU_DESCRIPTOR_HANDLE _cpuDsv)
{
    mSrvHandleCpu = _cpuSrv;
    mSrvHandleGpu = _gpuSrv;
    mDsvHandleCpu = _cpuDsv;

    buildDescriptors();

}

void ShadowMap::onResize(UINT _width, UINT _height)
{
    if (_width != width || _height != height)
    {
        width = _width;
        height = _height;

        buildResource();
        buildDescriptors();
    }


}

void ShadowMap::updateShadowTransform()
{

    /*TODO use main light*/

    XMVECTOR lightDir = XMLoadFloat3(&mRotatedLightDirections[0]);
    XMVECTOR lightPos = -2.0f * sceneBounds.Radius * lightDir;
    XMVECTOR targetPos = XMLoadFloat3(&sceneBounds.Center);
    XMVECTOR lightUp = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    XMMATRIX lightView = XMMatrixLookAtLH(lightPos, targetPos, lightUp);

    XMStoreFloat3(&mLightPosW, lightPos);

    // Transform bounding sphere to light space.
    XMFLOAT3 sphereCenterLS;
    XMStoreFloat3(&sphereCenterLS, XMVector3TransformCoord(targetPos, lightView));

    // Ortho frustum in light space encloses scene.
    float l = sphereCenterLS.x - sceneBounds.Radius;
    float b = sphereCenterLS.y - sceneBounds.Radius;
    float n = sphereCenterLS.z - sceneBounds.Radius;
    float r = sphereCenterLS.x + sceneBounds.Radius;
    float t = sphereCenterLS.y + sceneBounds.Radius;
    float f = sphereCenterLS.z + sceneBounds.Radius;

    mLightNearZ = n;
    mLightFarZ = f;
    XMMATRIX lightProj = XMMatrixOrthographicOffCenterLH(l, r, b, t, n, f);

    // Transform NDC space [-1,+1]^2 to texture space [0,1]^2
    XMMATRIX T(
        0.5f, 0.0f, 0.0f, 0.0f,
        0.0f, -0.5f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.5f, 0.5f, 0.0f, 1.0f);

    XMMATRIX S = lightView * lightProj * T;
    XMStoreFloat4x4(&mLightView, lightView);
    XMStoreFloat4x4(&mLightProj, lightProj);
    XMStoreFloat4x4(&mShadowTransform, S);


}

void ShadowMap::buildDescriptors()
{

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Texture2D.MipLevels = 1;
    srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
    srvDesc.Texture2D.PlaneSlice = 0;
    mDevice->CreateShaderResourceView(mShadowMap.Get(), &srvDesc, mSrvHandleCpu);

    D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
    dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
    dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    dsvDesc.Texture2D.MipSlice = 0;
    mDevice->CreateDepthStencilView(mShadowMap.Get(), &dsvDesc, mDsvHandleCpu);

}

void ShadowMap::buildResource()
{

    D3D12_RESOURCE_DESC mapDesc;
    ZeroMemory(&mapDesc, sizeof(D3D12_RESOURCE_DESC));

    mapDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    mapDesc.Alignment = 0;
    mapDesc.Width = width;
    mapDesc.Height = height;
    mapDesc.DepthOrArraySize = 1;
    mapDesc.MipLevels = 1;
    mapDesc.Format = mFormat;
    mapDesc.SampleDesc.Count = 1;
    mapDesc.SampleDesc.Quality = 0;
    mapDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    mapDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;


    D3D12_CLEAR_VALUE optClear;
    optClear.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    optClear.DepthStencil.Depth = 1.0f;
    optClear.DepthStencil.Stencil = 0;

    ThrowIfFailed(mDevice->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
        D3D12_HEAP_FLAG_NONE,
        &mapDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        &optClear,
        IID_PPV_ARGS(&mShadowMap)
    ));

}
