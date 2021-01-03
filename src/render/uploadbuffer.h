#pragma once

#include "../util/d3dUtil.h"

template<typename T>
class UploadBuffer
{
public:
    UploadBuffer(ID3D12Device* _device, UINT _elementCount, bool isConstantBuffer) :
        mIsConstantBuffer(isConstantBuffer)
    {
        if (isConstantBuffer)
        {
            mElementByteSize = d3dUtil::CalcConstantBufferSize(sizeof(T));
        }
        else
        {
            mElementByteSize = sizeof(T);
        }

        mElementCount = _elementCount;
        
        const auto heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
        const auto resDesc = CD3DX12_RESOURCE_DESC::Buffer((INT_PTR)mElementByteSize * _elementCount);

        ThrowIfFailed(_device->CreateCommittedResource(
            &heapProps,
            D3D12_HEAP_FLAG_NONE,
            &resDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&mUploadBuffer)));

        ThrowIfFailed(mUploadBuffer->Map(0, nullptr, reinterpret_cast<void**>(&mMappedData)));
    }

    UploadBuffer(const UploadBuffer& rhs) = delete;
    UploadBuffer& operator=(const UploadBuffer& rhs) = delete;
    ~UploadBuffer()
    {
        if (mUploadBuffer != nullptr)
            mUploadBuffer->Unmap(0, nullptr);

        mMappedData = nullptr;
    }

    ID3D12Resource* getResource()const
    {
        return mUploadBuffer.Get();
    }

    void copyData(int _elementIndex, const T& data)
    {
            memcpy(&mMappedData[_elementIndex * mElementByteSize], &data, sizeof(T));
    }

    void copyAll(const T& data)
    {
            memcpy(&mMappedData[0], &data, mElementCount * sizeof(T));
    }

private:
    Microsoft::WRL::ComPtr<ID3D12Resource> mUploadBuffer;
    BYTE* mMappedData = nullptr;

    UINT mElementCount = 0;
    UINT mElementByteSize = 0;
    bool mIsConstantBuffer = false;
};