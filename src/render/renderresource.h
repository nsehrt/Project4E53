#pragma once

#include "../util/d3dUtil.h"
#include "../util/geogen.h"
#include "frameresource.h"
#include <filesystem>


#define MODEL_PATH "data/models"
#define TEXTURE_PATH "data/textures"

using Microsoft::WRL::ComPtr;
using namespace DirectX;
using namespace DirectX::PackedVector;

class RenderResource
{

public:
    explicit RenderResource() = default;
    ~RenderResource() = default;
    RenderResource(const RenderResource& rhs) = delete;
    RenderResource& operator=(const RenderResource& rhs) = delete;

    void init(ID3D12Device* _device, ID3D12GraphicsCommandList* _cmdList, const std::filesystem::path& _texturePath, const std::filesystem::path& _modelPath);

private:
    ID3D12Device* device = nullptr;
    ID3D12GraphicsCommandList* cmdList = nullptr;

    ComPtr<ID3D12RootSignature> mMainRootSignature = nullptr;
    ComPtr<ID3D12DescriptorHeap> mSrvDescriptorHeap = nullptr;

    std::unordered_map<std::string, std::unique_ptr<Texture>> mTextures;
    std::unordered_map<std::string, std::unique_ptr<Mesh>> mMeshes;
    std::unordered_map<std::string, std::unique_ptr<Material>> mMaterials;

    std::unordered_map <std::string, ComPtr<ID3DBlob>> mShaders;
    std::unordered_map <std::string, std::vector<D3D12_INPUT_ELEMENT_DESC>> mInputLayouts;
    
    std::unordered_map <std::string, ComPtr<ID3D12PipelineState>> mPSOs;

    UINT mHeapDescriptorSize = 0;

    /*frame resource, camera, pass constants ??*/


    /*private init functions*/
    bool loadTexture(const std::string& file, TextureType type = TextureType::Texture2D);
    bool loadModel(const std::string& file);

    bool buildRootSignature();
    bool buildDescriptorHeap();
    std::array<const CD3DX12_STATIC_SAMPLER_DESC, 6> GetStaticSamplers();

    void buildShaders();
    void buildInputLayouts();

    void generateDefaultShapes();

    void buildPSOs();
    void buildMaterials();
    void buildRenderItems();
};