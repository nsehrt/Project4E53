#include "sobel.h"

void Sobel::execute(ID3D12GraphicsCommandList* cmdList, ID3D12RootSignature* rootSignature, ID3D12PipelineState* pso, CD3DX12_GPU_DESCRIPTOR_HANDLE input)
{

	cmdList->SetComputeRootSignature(rootSignature);
	cmdList->SetPipelineState(pso);
	cmdList->SetComputeRootDescriptorTable(0, input);
	cmdList->SetComputeRootDescriptorTable(2, mGpuUavHandle);
	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mOutput.Get(), D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));

	UINT threadGroupsX = (UINT)ceilf(mWidth / 16.0f);
	UINT threadGroupsY = (UINT)ceilf(mHeight / 16.0f);
	cmdList->Dispatch(threadGroupsX, threadGroupsY, 1);

	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mOutput.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_GENERIC_READ));

}

void Sobel::buildDescriptors(CD3DX12_CPU_DESCRIPTOR_HANDLE cpuDescHandle, CD3DX12_GPU_DESCRIPTOR_HANDLE gpuDescHandle, UINT descriptorSize)
{

    mCpuSrvHandle = cpuDescHandle;
    mCpuUavHandle = cpuDescHandle.Offset(1, descriptorSize);

    mGpuSrvHandle = gpuDescHandle;
    mGpuUavHandle = gpuDescHandle.Offset(1, descriptorSize);

    buildDescriptors();
}

void Sobel::onResize(UINT width, UINT height)
{
    if (mWidth != width || mHeight != height)
    {
        mWidth = width;
        mHeight = height;

        buildResource();
        buildDescriptors();
    }


}

void Sobel::buildDescriptors()
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

	mDevice->CreateShaderResourceView(mOutput.Get(), &srvDesc, mCpuSrvHandle);
	mDevice->CreateUnorderedAccessView(mOutput.Get(), nullptr, &uavDesc, mCpuUavHandle);

}

void Sobel::buildResource()
{

	D3D12_RESOURCE_DESC texDesc;
	ZeroMemory(&texDesc, sizeof(D3D12_RESOURCE_DESC));
	texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	texDesc.Alignment = 0;
	texDesc.Width = mWidth;
	texDesc.Height = mHeight;
	texDesc.DepthOrArraySize = 1;
	texDesc.MipLevels = 1;
	texDesc.Format = mFormat;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

	ThrowIfFailed(mDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&texDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&mOutput)));

}
