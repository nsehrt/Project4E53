#pragma once

#include <Windows.h>
#include <wrl.h>
#include <dxgi1_6.h>
#include <d3d12.h>
#include <d3dcompiler.h>
#include <DirectXPackedVector.h>
#include <DirectXCollision.h>
#include <string>
#include <unordered_map>
#include <iostream>
#include <sstream>
#include "../util/mathhelper.h"
#include "../extern/json.hpp"

using json = nlohmann::json;

/*structs for materials, textures etc.*/

extern int gNumFrameResources;

struct Material
{
    // Unique material name for lookup.
    std::string Name;

    // Index into constant buffer corresponding to this material.
    int MatCBIndex = -1;

    // Index into SRV heap for diffuse texture.
    int DiffuseSrvHeapIndex = -1;

    // Index into SRV heap for normal texture.
    int NormalSrvHeapIndex = -1;

    int Displacement1HeapIndex = -1;
    int Displacement2HeapIndex = -1;

    int MiscTexture1Index = -1;
    int MiscTexture2Index = -1;

    int NumFramesDirty = gNumFrameResources;

    // Material constant buffer data used for shading.
    DirectX::XMFLOAT4 DiffuseAlbedo = { 1.0f, 1.0f, 1.0f, 1.0f };
    DirectX::XMFLOAT3 FresnelR0 = { 0.01f, 0.01f, 0.01f };
    float Roughness = .25f;
    DirectX::XMFLOAT4X4 MatTransform = MathHelper::identity4x4();
    DirectX::XMFLOAT4X4 DisplacementTransform0 = MathHelper::identity4x4();
    DirectX::XMFLOAT4X4 DisplacementTransform1 = MathHelper::identity4x4();
    DirectX::XMFLOAT4X4 NormalTransform0 = MathHelper::identity4x4();
    DirectX::XMFLOAT4X4 NormalTransform1 = MathHelper::identity4x4();
    float MiscFloat1 = 0.0f;
    float MiscFloat2 = 0.0f;
};

struct Mesh
{
    std::string materialName;

    Material* material = nullptr;

    Microsoft::WRL::ComPtr<ID3D12Resource> VertexBufferGPU = nullptr;
    Microsoft::WRL::ComPtr<ID3D12Resource> IndexBufferGPU = nullptr;

    Microsoft::WRL::ComPtr<ID3D12Resource> VertexBufferUploader = nullptr;
    Microsoft::WRL::ComPtr<ID3D12Resource> IndexBufferUploader = nullptr;

    // Data about the buffers.
    UINT VertexByteStride = 0;
    UINT VertexBufferByteSize = 0;
    DXGI_FORMAT IndexFormat = DXGI_FORMAT_R16_UINT;
    UINT IndexBufferByteSize = 0;

    UINT IndexCount = 0;

    D3D12_VERTEX_BUFFER_VIEW VertexBufferView()const
    {
        D3D12_VERTEX_BUFFER_VIEW vbv{};
        vbv.BufferLocation = VertexBufferGPU->GetGPUVirtualAddress();
        vbv.StrideInBytes = VertexByteStride;
        vbv.SizeInBytes = VertexBufferByteSize;

        return vbv;
    }

    D3D12_INDEX_BUFFER_VIEW IndexBufferView()const
    {
        D3D12_INDEX_BUFFER_VIEW ibv{};
        ibv.BufferLocation = IndexBufferGPU->GetGPUVirtualAddress();
        ibv.Format = IndexFormat;
        ibv.SizeInBytes = IndexBufferByteSize;

        return ibv;
    }

    // We can free this memory after we finish upload to the GPU.
    void DisposeUploaders()
    {
        VertexBufferUploader = nullptr;
        IndexBufferUploader = nullptr;
    }
};

/*static model*/
struct Model
{
    std::string name;

    std::string group;

    std::vector<std::unique_ptr<Mesh>> meshes;

    DirectX::BoundingBox baseModelBox;

    std::unique_ptr<Mesh> boundingBoxMesh = nullptr;
};


/*animation clips*/
struct KeyFrame
{
    explicit KeyFrame() : timeStamp(0.0f),
        translation(0.0f, 0.0f, 0.0f),
        scale(1.0f, 1.0f, 1.0f),
        rotationQuat(0.0f, 0.0f, 0.0f, 1.0f)
    {
    }
    ~KeyFrame() = default;

    float timeStamp;
    DirectX::XMFLOAT3 translation;
    DirectX::XMFLOAT3 scale;
    DirectX::XMFLOAT4 rotationQuat;
};

struct BoneAnimation
{
    std::vector<KeyFrame> keyFrames;
    bool isEmpty = false;

    float getStartTime() const
    {
        if (isEmpty) return D3D12_FLOAT32_MAX;

        return keyFrames.front().timeStamp;
    }

    float getEndTime() const
    {
        if (isEmpty) return -D3D12_FLOAT32_MAX;

        return keyFrames.back().timeStamp;
    }

    void interpolate(float time, DirectX::XMFLOAT4X4& matrix) const;
    void interpolate(float time, KeyFrame& keyFrame) const;
};


struct AnimationClip
{
    std::vector<BoneAnimation> boneAnimations;
    std::string name;

private:
    float startTime = -1.0f;
    float endTime = -1.0f;

public:

    float getStartTime();
    float getEndTime();
    void interpolate(float t, std::vector<DirectX::XMFLOAT4X4>& boneTransforms) const;
    void interpolate(float t, std::vector<KeyFrame>& keyTransforms) const;
};

struct Node
{
    std::string name = "";
    Node* parent = nullptr;

    bool isBone = false;
    int boneIndex = -1;

    DirectX::XMFLOAT4X4 transform = {};
    DirectX::XMFLOAT4X4 boneOffset = {};

    DirectX::XMFLOAT4X4 globalTransform = {};

    std::vector<Node*> children;

    ~Node()
    {
        for (auto i = 0; i < children.size(); i++)
        {
            delete children[i];
        }
    }
};

struct NodeTree
{
    NodeTree()
    {
        root = new Node();
    }

    ~NodeTree()
    {
        delete root;
    }


    Node* root = nullptr;
    Node* boneRoot = nullptr;

    Node* findNodeByBoneIndex(int index) const;
    std::string toString();

private:
    void printNodes(std::stringstream& str, Node* node, int depth = 0);
};

/*skinned model*/
struct SkinnedModel : Model
{
    /*bone information*/
    UINT boneCount = 0;
    std::vector<int> boneHierarchy;
    NodeTree nodeTree;
    DirectX::XMFLOAT4X4 rootTransform = {};

    void calculateFinalTransforms(AnimationClip* currentClip, std::vector<DirectX::XMFLOAT4X4>& finalTransforms, float timePos);
};



struct Light
{
    DirectX::XMFLOAT3 Strength = { 0.0f, 0.0f, 0.0f };
    float FalloffStart = 1.0f;                          // point/spot light only
    DirectX::XMFLOAT3 Direction = { 0.0f, -1.0f, 0.0f };// directional/spot light only
    float FalloffEnd = 10.0f;                           // point/spot light only
    DirectX::XMFLOAT3 Position = { 0.0f, 0.0f, 0.0f };  // point/spot light only
    float SpotPower = 64.0f;                            // spot light only
};

struct MaterialConstants
{
    DirectX::XMFLOAT4 DiffuseAlbedo = { 1.0f, 1.0f, 1.0f, 1.0f };
    DirectX::XMFLOAT3 FresnelR0 = { 0.01f, 0.01f, 0.01f };
    float Roughness = 0.25f;

    // Used in texture mapping.
    DirectX::XMFLOAT4X4 MatTransform = MathHelper::identity4x4();
};

enum class TextureType
{
    Texture2D,
    TextureCube
};

struct Texture
{
    std::string Name;
    std::wstring Filename;
    Microsoft::WRL::ComPtr<ID3D12Resource> Resource = nullptr;
    Microsoft::WRL::ComPtr<ID3D12Resource> UploadHeap = nullptr;
    TextureType Type = TextureType::Texture2D;

    UINT index = 0;
};

enum class RenderType
{
    Terrain,
    TerrainWireFrame,
    Water,
    Grass,
    Default,
    DefaultNoNormal,
    NoCullNoNormal,
    DefaultAlpha,
    SkinnedDefault,
    SkinnedNone,
    Sky, 
    Outline,/*everything before transparent objects*/
    Particle_Smoke,
    Particle_Fire,
    DefaultTransparency,
    COUNT
};

enum class ShadowRenderType
{
    ShadowDefault,
    ShadowAlpha,
    ShadowSkinned,
    ShadowSkinnedAlpha,
    COUNT
};

enum class PostProcessRenderType
{
    Debug,
    Outline,
    Hitbox,
    Composite,
    CompositeMult,
    Sobel,
    BlurHorz,
    BlurVert,
    COUNT
};

struct RenderItem
{
    RenderItem() = default;
    RenderItem(const RenderItem& rhs) = delete;

    DirectX::XMFLOAT4X4 World = MathHelper::identity4x4();
    DirectX::XMFLOAT4X4 TexTransform = MathHelper::identity4x4();

    int NumFramesDirty = gNumFrameResources;

    // index into constant buffer for object data and skinned data
    std::vector<UINT>ObjCBIndex;
    UINT SkinnedCBIndex = 0;

    Model* staticModel = nullptr;
    SkinnedModel* skinnedModel = nullptr;

    AnimationClip* currentClip = nullptr;
    std::vector<DirectX::XMFLOAT4X4> finalTransforms;
    Material* MaterialOverwrite = nullptr;

    float animationTimer = 0.0f;

    bool isSkinned() const
    {
        if (!skinnedModel)
        {
            return false;
        }
        else
        {
            return true;
        }
    }

    Model* getModel() const
    {
        return (isSkinned() ? skinnedModel : staticModel);
    }

    // Primitive topology.
    D3D12_PRIMITIVE_TOPOLOGY PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

    RenderType renderType = RenderType::Default;
    ShadowRenderType shadowType = ShadowRenderType::ShadowDefault;
};