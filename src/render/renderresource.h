#pragma once

#include "../util/d3dUtil.h"
#include "../util/geogen.h" 
#include "../core/camera.h"
#include "../core/gametime.h"
#include "../render/frameresource.h"
#include "../render/shadowmap.h"
#include <filesystem>


#define MODEL_PATH "data/model"
#define TEXTURE_PATH "data/texture"

using Microsoft::WRL::ComPtr;
using namespace DirectX;
using namespace DirectX::PackedVector;

class RenderResource
{

public:
    explicit RenderResource(ComPtr<ID3D12DescriptorHeap> rtvHeap, ComPtr<ID3D12DescriptorHeap> dsvHeap)
    {
        mRtvHeap = rtvHeap;
        mDsvHeap = dsvHeap;
    };

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

    /*frame resource related*/

    /*use next frame resource (start of new frame) */
    void cycleFrameResource();

    /*return the index of the current frame resource */
    int getCurrentFrameResourceIndex();

    /* return pointer to the current frame resource */
    FrameResource* getCurrentFrameResource();

    void updateBuffers(const GameTime& gt);

    /*resources*/
    std::unordered_map <std::string, ComPtr<ID3D12PipelineState>> mPSOs;
    std::unordered_map<std::string, std::unique_ptr<Texture>> mTextures;
    std::unordered_map<std::string, std::unique_ptr<Model>> mModels;
    std::unordered_map<std::string, std::unique_ptr<Material>> mMaterials;
    std::unordered_map <std::string, ComPtr<ID3DBlob>> mShaders;
    std::vector<std::vector<D3D12_INPUT_ELEMENT_DESC>> mInputLayouts;

    UINT mRtvDescriptorSize = 0;
    UINT mDsvDescriptorSize = 0;
    UINT mCbvSrvUavDescriptorSize = 0;


    ID3D12Device* device = nullptr;
    ID3D12GraphicsCommandList* cmdList = nullptr;
    ComPtr<ID3D12RootSignature> mMainRootSignature = nullptr;
    ComPtr<ID3D12DescriptorHeap> mSrvDescriptorHeap = nullptr;
    ComPtr<ID3D12DescriptorHeap> mRtvHeap = nullptr;
    ComPtr<ID3D12DescriptorHeap> mDsvHeap = nullptr;
    
    ShadowMap* getShadowMap()
    {
        return mShadowMap.get();
    }
    CD3DX12_GPU_DESCRIPTOR_HANDLE mNullSrv;

private:

    /*shadow map*/

    std::unique_ptr<ShadowMap> mShadowMap = nullptr;

    float mLightNearZ = 0.0f;
    float mLightFarZ = 0.0f;
    XMFLOAT3 mLightPosW;
    XMFLOAT4X4 mLightView = MathHelper::identity4x4();
    XMFLOAT4X4 mLightProj = MathHelper::identity4x4();
    XMFLOAT4X4 mShadowTransform = MathHelper::identity4x4();

    float mLightRotationAngle = 0.0f;
    XMFLOAT3 mBaseLightDirections[3] = {
        XMFLOAT3(0.57735f, -0.57735f, 0.57735f),
        XMFLOAT3(-0.57735f, -0.57735f, 0.57735f),
        XMFLOAT3(0.0f, -0.707f, -0.707f)
    };
    XMFLOAT3 mRotatedLightDirections[3];

    UINT mNullCubeSrvIndex = 0;
    UINT mNullTexSrvIndex = 0;

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

    /*frame resource related*/
    std::vector<std::unique_ptr<FrameResource>> mFrameResources;
    FrameResource* mCurrentFrameResource = nullptr;
    int mCurrentFrameResourceIndex = 0;

    PassConstants mMainPassConstants;
    PassConstants mShadowPassConstants;

    const UINT MAX_GAME_OBJECTS = 512;

    void buildFrameResource();

    void updateShadowTransform(const GameTime& gt);

    void updateGameObjectConstantBuffers(const GameTime& gt);
    void updateMainPassConstantBuffers(const GameTime& gt);
    void updateMaterialConstantBuffers(const GameTime& gt);
    void updateShadowPassConstantBuffers(const GameTime& gt);
};