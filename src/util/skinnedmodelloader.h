#pragma once

#include "../render/renderstructs.h"
#include "d3dUtil.h"
#include "../render/renderresource.h"
#include "serviceprovider.h"
#include <filesystem>

class SkinnedModelLoader
{
public:
    explicit SkinnedModelLoader(ID3D12Device* _device, ID3D12GraphicsCommandList* _cmdList)
    {
        device = _device;
        cmdList = _cmdList;
    }
    ~SkinnedModelLoader() = default;

    std::unique_ptr<Model> loadS3D(const std::filesystem::directory_entry& fileName);

private:
    ID3D12Device* device;
    ID3D12GraphicsCommandList* cmdList;
};