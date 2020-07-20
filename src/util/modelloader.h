#pragma once

#include "d3dUtil.h"
#include <filesystem>

class ModelLoader
{
public:
    explicit ModelLoader(ID3D12Device* _device, ID3D12GraphicsCommandList* _cmdList)
    {
        device = _device;
        cmdList = _cmdList;
    }
    ~ModelLoader() = default;

    std::unique_ptr<Model> loadB3D(const std::filesystem::directory_entry& file);

private:
    ID3D12Device* device;
    ID3D12GraphicsCommandList* cmdList;
};