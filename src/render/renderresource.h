#pragma once

#include "../util/d3dUtil.h"
#include "../util/geogen.h"
#include "../core/camera.h"
#include "../core/gametime.h"
#include "../render/frameresource.h"
#include "../render/shadowmap.h"
#include "../render/rendertarget.h"
#include "../render/sobel.h"
#include "../render/blur.h"
#include <filesystem>

#define MODEL_PATH "data/model"
#define TEXTURE_PATH "data/texture"
#define SKINNED_PATH "data/skinned"
#define ANIM_PATH "data/anim"

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
    bool init(ID3D12Device* _device, ID3D12GraphicsCommandList* _cmdList, const std::filesystem::path& _texturePath, const std::filesystem::path& _modelPath, const std::filesystem::path& _skinnedPath, const std::filesystem::path& _animPath);

    void onResize();

    void toggleRoughHitBoxDraw()
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

    void setPSO(RenderType renderType);
    ID3D12PipelineState* getPSO(RenderType renderType);

    /*resources*/
    std::unordered_map<std::string, std::unique_ptr<Texture>> mTextures;
    std::unordered_map<std::string, std::unique_ptr<Model>> mModels;
    std::unordered_map<std::string, std::unique_ptr<SkinnedModel>> mSkinnedModels;
    std::unordered_map<std::string, std::unique_ptr<Material>> mMaterials;
    std::unordered_map < std::string, std::unique_ptr<AnimationClip>> mAnimations;

    UINT mRtvDescriptorSize = 0;
    UINT mDsvDescriptorSize = 0;
    UINT mCbvSrvUavDescriptorSize = 0;

    ID3D12Device* device = nullptr;
    ID3D12GraphicsCommandList* cmdList = nullptr;
    ID3D12CommandQueue* cmdQueue = nullptr;

    ComPtr<ID3D12RootSignature> mMainRootSignature = nullptr;
    ComPtr<ID3D12RootSignature> mPostProcessRootSignature = nullptr;
    ComPtr<ID3D12RootSignature> mTerrainRootSignature = nullptr;

    ComPtr<ID3D12DescriptorHeap> mSrvDescriptorHeap = nullptr;
    ComPtr<ID3D12DescriptorHeap> mRtvHeap = nullptr;
    ComPtr<ID3D12DescriptorHeap> mDsvHeap = nullptr;


    ShadowMap* getShadowMap()
    {
        return mShadowMap.get();
    }

    RenderTarget* getRenderTarget()
    {
        return mRenderTarget.get();
    }

    Sobel* getSobelFilter()
    {
        return mSobelFilter.get();
    }

    Blur* getBlurFilter()
    {
        return mBlurFilter.get();
    }

    std::vector<float>& getCompositeColor()
    {
        return mCompositeColor;
    }

    bool useMultColor = false;

    std::vector<float>& getMultColor()
    {
        return mMultColor;
    }

    void addToCompositeColor(float f)
    {
        
        for (int i = 0; i < 3; i++)
        {
            mCompositeColor[i] = MathHelper::clampH(mCompositeColor[i] + f, 0.0f, 1.0f);
        }

    }

    CD3DX12_GPU_DESCRIPTOR_HANDLE mNullSrv;

private:

    std::unordered_map <RenderType, ComPtr<ID3D12PipelineState>> mPSOs;
    std::unordered_map <std::string, ComPtr<ID3DBlob>> mShaders;
    std::vector<std::vector<D3D12_INPUT_ELEMENT_DESC>> mInputLayouts;

    /*post process*/
    std::unique_ptr<RenderTarget> mRenderTarget = nullptr;
    std::unique_ptr<Sobel> mSobelFilter = nullptr;
    std::unique_ptr<Blur> mBlurFilter = nullptr;
    std::vector<float> mCompositeColor = { 0.0f,0.0f,0.0f,0.0f };
    std::vector<float> mMultColor = { 0.3f, 0.59f, 0.11f };

    /*shadow map*/

    std::unique_ptr<ShadowMap> mShadowMap = nullptr;

    float mLightNearZ = 0.0f;
    float mLightFarZ = 0.0f;
    XMFLOAT3 mLightPosW;
    XMFLOAT4X4 mLightView = MathHelper::identity4x4();
    XMFLOAT4X4 mLightProj = MathHelper::identity4x4();
    XMFLOAT4X4 mShadowTransform = MathHelper::identity4x4();

    UINT mNullCubeSrvIndex = 0;
    UINT mNullTexSrvIndex = 0;

    /*private init functions*/
    bool loadTexture(const std::filesystem::directory_entry& file, TextureType type = TextureType::Texture2D);

    bool buildRootSignature();
    bool buildPostProcessSignature();
    bool buildTerrainRootSignature();

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

    const UINT MAX_GAME_OBJECTS = 8192;
    const UINT MAX_SKINNED_OBJECTS = 256;
    const UINT MAX_PARTICLE_SYSTEMS = 64;
    const UINT SHADOW_RADIUS = 40;

    void buildFrameResource();

    void updateShadowTransform(const GameTime& gt);

    void updateGameObjectConstantBuffers(const GameTime& gt);
    void updateMainPassConstantBuffers(const GameTime& gt);
    void updateMaterialConstantBuffers(const GameTime& gt);
    void updateShadowPassConstantBuffers(const GameTime& gt);



    bool exists(const nlohmann::json& j, const std::string& key)
    {
        return j.find(key) != j.end();
    }
};