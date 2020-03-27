#pragma once

#include "../util/d3dUtil.h"
#include "../util/geogen.h" 
#include "../core/camera.h"
#include "../core/gametime.h"
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

    bool init(ID3D12Device* _device, ID3D12GraphicsCommandList* _cmdList, const std::filesystem::path& _texturePath, const std::filesystem::path& _modelPath);

    void draw();

    void useDefaultCamera()
    {
        activeCamera = &defaultCamera;
    }

    void incFrameResource();
    void update(const GameTime& gt);
    int getCurrentFrameResourceIndex();
    FrameResource* getCurrentFrameResource();

    Camera* activeCamera = nullptr;

    ComPtr<ID3D12RootSignature> mMainRootSignature = nullptr;
    ComPtr<ID3D12DescriptorHeap> mSrvDescriptorHeap = nullptr;

    std::unordered_map <std::string, ComPtr<ID3D12PipelineState>> mPSOs;

    std::unordered_map<std::string, std::unique_ptr<Texture>> mTextures;
    std::unordered_map<std::string, std::unique_ptr<Mesh>> mMeshes;
    std::unordered_map<std::string, std::unique_ptr<Material>> mMaterials;

    std::unordered_map <std::string, ComPtr<ID3DBlob>> mShaders;
    std::unordered_map <std::string, std::vector<D3D12_INPUT_ELEMENT_DESC>> mInputLayouts;
    std::vector<std::unique_ptr<RenderItem>> mAllRitems;

    UINT mHeapDescriptorSize = 0;

private:
    ID3D12Device* device = nullptr;
    ID3D12GraphicsCommandList* cmdList = nullptr;

    /*frame resource, camera, pass constants ??*/
    std::vector<std::unique_ptr<FrameResource>> mFrameResources;
    FrameResource* mCurrentFrameResource = nullptr;
    int mCurrentFrameResourceIndex = 0;

    Camera defaultCamera;
    PassConstants mMainPassConstants;

    /*private init functions*/
    bool loadTexture(const std::filesystem::directory_entry& file, TextureType type = TextureType::Texture2D);
    bool loadModel(const std::string& file);

    bool buildRootSignature();
    bool buildDescriptorHeap();
    std::array<const CD3DX12_STATIC_SAMPLER_DESC, 6> GetStaticSamplers();

    void buildShaders();
    void buildInputLayouts();

    void generateDefaultShapes();

    void buildPSOs();
    bool buildMaterials();
    bool buildFrameResources();
    void buildRenderItems();


    void updateObjectCBs(const GameTime& gt);
    void updatePassCBs(const GameTime& gt);
    void updateMaterialBuffers(const GameTime& gt);
};