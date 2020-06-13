#pragma once

#include "../render/renderresource.h"
#include "serviceprovider.h"
#include <filesystem>

class ClipLoader
{
public:
    explicit ClipLoader(ID3D12Device* _device, ID3D12GraphicsCommandList* _cmdList)
    {
        device = _device;
        cmdList = _cmdList;
    }
    ~ClipLoader() = default;

    std::unique_ptr<AnimationClip> loadCLP(const std::filesystem::directory_entry& fileName);

private:
    ID3D12Device* device;
    ID3D12GraphicsCommandList* cmdList;

};