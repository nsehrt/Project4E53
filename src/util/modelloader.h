#pragma once

#include "../render/renderstructs.h"
#include "d3dUtil.h"
#include "../render/renderresource.h"
#include "serviceprovider.h"
#include <filesystem>

struct ModelReturn
{
    int errorCode = 0;
    std::unique_ptr<Model> model;
};

class ModelLoader
{
public:
    ModelLoader(ID3D12Device* _device, ID3D12GraphicsCommandList* _cmdList)
    {
        device = _device;
        cmdList = _cmdList;
    }
    ~ModelLoader() = default;

    ModelReturn loadB3D(const std::filesystem::directory_entry& file);

private:
    ID3D12Device* device;
    ID3D12GraphicsCommandList* cmdList;
};