#pragma once

#include "../util/d3dUtil.h"
#include "../util/geogen.h" 
#include "../core/camera.h"
#include "../core/gametime.h"
#include "frameresource.h"
#include <filesystem>


#define MODEL_PATH "data/model"
#define TEXTURE_PATH "data/texture"

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

    /*set up render resource*/
    bool init(ID3D12Device* _device, ID3D12GraphicsCommandList* _cmdList, const std::filesystem::path& _texturePath, const std::filesystem::path& _modelPath);

    void toggleHitBoxDraw()
    {
        drawHitbox = !drawHitbox;
    }

    bool isHitBoxDrawEnabled() const
    {
        return drawHitbox;
    }

    /*resources*/
    std::unordered_map <std::string, ComPtr<ID3D12PipelineState>> mPSOs;
    std::unordered_map<std::string, std::unique_ptr<Texture>> mTextures;
    std::unordered_map<std::string, std::unique_ptr<Model>> mModels;
    std::unordered_map<std::string, std::unique_ptr<Material>> mMaterials;
    std::unordered_map <std::string, ComPtr<ID3DBlob>> mShaders;
    std::unordered_map <std::string, std::vector<D3D12_INPUT_ELEMENT_DESC>> mInputLayouts;

    UINT mHeapDescriptorSize = 0;

    ID3D12Device* device = nullptr;
    ID3D12GraphicsCommandList* cmdList = nullptr;
    ComPtr<ID3D12RootSignature> mMainRootSignature = nullptr;
    ComPtr<ID3D12DescriptorHeap> mSrvDescriptorHeap = nullptr;


private:

    /*private init functions*/
    bool loadTexture(const std::filesystem::directory_entry& file, TextureType type = TextureType::Texture2D);

    bool buildRootSignature();
    bool buildDescriptorHeap();
    std::array<const CD3DX12_STATIC_SAMPLER_DESC, 7> GetStaticSamplers();

    void buildShaders();
    void buildInputLayouts();

    void generateDefaultShapes();

    void buildPSOs();
    bool buildMaterials();


    bool drawHitbox = false;
};