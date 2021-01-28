#include "blur.h"


void Blur::buildDescriptors(CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuDescriptor, CD3DX12_GPU_DESCRIPTOR_HANDLE hGpuDescriptor, UINT descriptorSize)
{
    mBlur0CpuSrv = hCpuDescriptor;
    mBlur0CpuUav = hCpuDescriptor.Offset(1, descriptorSize);
    mBlur1CpuSrv = hCpuDescriptor.Offset(1, descriptorSize);
    mBlur1CpuUav = hCpuDescriptor.Offset(1, descriptorSize);

    mBlur0GpuSrv = hGpuDescriptor;
    mBlur0GpuUav = hGpuDescriptor.Offset(1, descriptorSize);
    mBlur1GpuSrv = hGpuDescriptor.Offset(1, descriptorSize);
    mBlur1GpuUav = hGpuDescriptor.Offset(1, descriptorSize);

    buildDescriptors();
}

void Blur::execute(ID3D12GraphicsCommandList* cmdList, ID3D12RootSignature* rootSig, ID3D12PipelineState* horzBlurPSO, ID3D12PipelineState* vertBlurPSO, ID3D12Resource* input)
{

    auto weights = calculateWeights(mSigma);
    int blurRadius = (int)weights.size() / 2;

    cmdList->SetComputeRootSignature(rootSig);

    cmdList->SetComputeRoot32BitConstants(0, 1, &blurRadius, 0);
    cmdList->SetComputeRoot32BitConstants(0, (UINT)weights.size(), weights.data(), 1);

    const auto resBarr = CD3DX12_RESOURCE_BARRIER::Transition(input,
                                                              D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_SOURCE);
    cmdList->ResourceBarrier(1, &resBarr);

    const auto resBarr2 = CD3DX12_RESOURCE_BARRIER::Transition(mBlurMap0.Get(),
                                                              D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
    cmdList->ResourceBarrier(1, &resBarr2);

    // Copy the input (back-buffer in this example) to BlurMap0.
    cmdList->CopyResource(mBlurMap0.Get(), input);

    const auto resBarr3 = CD3DX12_RESOURCE_BARRIER::Transition(mBlurMap0.Get(),
                                                               D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ);
    cmdList->ResourceBarrier(1, &resBarr3);

    const auto resBarr4 = CD3DX12_RESOURCE_BARRIER::Transition(mBlurMap1.Get(),
                                                               D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    cmdList->ResourceBarrier(1, &resBarr4);

    for (UINT i = 0; i < blurIterations; ++i)
    {
        //
        // Horizontal Blur pass.
        //

        cmdList->SetPipelineState(horzBlurPSO);

        cmdList->SetComputeRootDescriptorTable(1, mBlur0GpuSrv);
        cmdList->SetComputeRootDescriptorTable(3, mBlur1GpuUav);

        // How many groups do we need to dispatch to cover a row of pixels, where each
        // group covers 256 pixels (the 256 is defined in the ComputeShader).
        UINT numGroupsX = (UINT)ceilf(width / 256.0f);
        cmdList->Dispatch(numGroupsX, height, 1);

        const auto resBarr = CD3DX12_RESOURCE_BARRIER::Transition(mBlurMap0.Get(),
                                                                  D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        cmdList->ResourceBarrier(1, &resBarr);

        const auto resBarr2 = CD3DX12_RESOURCE_BARRIER::Transition(mBlurMap1.Get(),
                                                                   D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_GENERIC_READ);
        cmdList->ResourceBarrier(1, &resBarr2);

        //
        // Vertical Blur pass.
        //

        cmdList->SetPipelineState(vertBlurPSO);

        cmdList->SetComputeRootDescriptorTable(1, mBlur1GpuSrv);
        cmdList->SetComputeRootDescriptorTable(3, mBlur0GpuUav);

        // How many groups do we need to dispatch to cover a column of pixels, where each
        // group covers 256 pixels  (the 256 is defined in the ComputeShader).
        UINT numGroupsY = (UINT)ceilf(height / 256.0f);
        cmdList->Dispatch(width, numGroupsY, 1);

        const auto resBarr3 = CD3DX12_RESOURCE_BARRIER::Transition(mBlurMap0.Get(),
                                                                   D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COMMON);
        cmdList->ResourceBarrier(1, &resBarr3);

        const auto resBarr4 = CD3DX12_RESOURCE_BARRIER::Transition(mBlurMap1.Get(),
                                                                   D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_COMMON);
        cmdList->ResourceBarrier(1, &resBarr4);
    }

}

std::vector<float> Blur::calculateWeights(float sigma)
{
    float twoSigma = 2.0f * sigma * sigma;
    int radius = (int)ceil(2.0f * sigma);

    ASSERT(radius <= MaxRadius);

    std::vector<float> weights;
    weights.resize((INT_PTR)2 * radius + 1);

    float weightSum = 0.0f;

    for (int i = -radius; i <= radius; i++)
    {
        float x = (float)i;
        weights[(INT_PTR)i + radius] = expf(-x * x / twoSigma);
        weightSum += weights[(INT_PTR)i + radius];
    }

    /*normalize weights*/
    for (int i = 0; i < weights.size(); i++)
    {
        weights[i] /= weightSum;
    }

    return weights;
}

void Blur::buildDescriptors()
{
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Format = mFormat;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Texture2D.MipLevels = 1;

    D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};

    uavDesc.Format = mFormat;
    uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
    uavDesc.Texture2D.MipSlice = 0;

    mDevice->CreateShaderResourceView(mBlurMap0.Get(), &srvDesc, mBlur0CpuSrv);
    mDevice->CreateUnorderedAccessView(mBlurMap0.Get(), nullptr, &uavDesc, mBlur0CpuUav);

    mDevice->CreateShaderResourceView(mBlurMap1.Get(), &srvDesc, mBlur1CpuSrv);
    mDevice->CreateUnorderedAccessView(mBlurMap1.Get(), nullptr, &uavDesc, mBlur1CpuUav);
}

void Blur::buildResources()
{

    D3D12_RESOURCE_DESC texDesc{};
    ZeroMemory(&texDesc, sizeof(D3D12_RESOURCE_DESC));
    texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    texDesc.Alignment = 0;
    texDesc.Width = width;
    texDesc.Height = height;
    texDesc.DepthOrArraySize = 1;
    texDesc.MipLevels = 1;
    texDesc.Format = mFormat;
    texDesc.SampleDesc.Count = 1;
    texDesc.SampleDesc.Quality = 0;
    texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

    const auto heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    ThrowIfFailed(mDevice->CreateCommittedResource(
        &heapProp,
        D3D12_HEAP_FLAG_NONE,
        &texDesc,
        D3D12_RESOURCE_STATE_COMMON,
        nullptr,
        IID_PPV_ARGS(&mBlurMap0)));

    ThrowIfFailed(mDevice->CreateCommittedResource(
        &heapProp,
        D3D12_HEAP_FLAG_NONE,
        &texDesc,
        D3D12_RESOURCE_STATE_COMMON,
        nullptr,
        IID_PPV_ARGS(&mBlurMap1)));

}
