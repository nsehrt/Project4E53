#include "frameresource.h"

FrameResource::FrameResource(ID3D12Device* device,
                             UINT passCount,
                             UINT objectCount,
                             UINT skinnedObjectCount,
                             UINT materialCount,
                             UINT terrainVertexCount,
                             UINT particleSystemCount)
{
    ThrowIfFailed(device->CreateCommandAllocator(
        D3D12_COMMAND_LIST_TYPE_DIRECT,
        IID_PPV_ARGS(CmdListAlloc.GetAddressOf())));

    PassCB = std::make_unique<UploadBuffer<PassConstants>>(device, passCount, true);
    MaterialBuffer = std::make_unique<UploadBuffer<MaterialData>>(device, materialCount, false);
    ObjectCB = std::make_unique<UploadBuffer<ObjectConstants>>(device, objectCount, true);
    TerrainVB = std::make_unique<UploadBuffer<TerrainVertex>>(device, terrainVertexCount, false);
    SkinnedCB = std::make_unique<UploadBuffer<SkinnedConstants>>(device, skinnedObjectCount, true);

    ParticleVB.resize(particleSystemCount);

    for (UINT i = 0; i < ParticleVB.size(); i++)
    {
        ParticleVB[i] = std::make_unique<UploadBuffer<ParticleVertex>>(device, 5000, false);
    }

}

FrameResource::~FrameResource()
{
}