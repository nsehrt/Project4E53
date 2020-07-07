#pragma once

#include <Windows.h>
#include <wrl.h>
#include <dxgi1_6.h>
#include <d3d12.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
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
        D3D12_VERTEX_BUFFER_VIEW vbv;
        vbv.BufferLocation = VertexBufferGPU->GetGPUVirtualAddress();
        vbv.StrideInBytes = VertexByteStride;
        vbv.SizeInBytes = VertexBufferByteSize;

        return vbv;
    }

    D3D12_INDEX_BUFFER_VIEW IndexBufferView()const
    {
        D3D12_INDEX_BUFFER_VIEW ibv;
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

    DirectX::BoundingOrientedBox boundingBox;
    DirectX::BoundingBox frustumBoundingBox;

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

    float getStartTime() const
    {
        return keyFrames.front().timeStamp;
    }

    float getEndTime() const
    {
        return keyFrames.back().timeStamp;
    }

    void interpolate(float time, DirectX::XMFLOAT4X4& matrix) const;

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
};

struct Node
{
    std::string name = "";
    Node* parent = nullptr;

    DirectX::XMFLOAT4X4 transform = {};

    bool isBone = false;
    DirectX::XMFLOAT4X4 boneOffset = {};

    std::vector<std::unique_ptr<Node>> children;
};

static void printNodes(std::stringstream& str, Node* node, int depth = 0)
{
    str << std::string((long long)depth * 3, ' ') << std::string(2, '-') << ">" << node->name << (node->isBone ? " (Bone)" : " (Not a bone)");

    if (node->parent != nullptr)
    {
        str << " child of node " << node->parent->name;
    }
    else
    {
        str << " root node";
    }

    DirectX::XMMATRIX trf = DirectX::XMLoadFloat4x4(&node->transform);

    if (DirectX::XMMatrixIsIdentity(trf))
    {
        str << " (Identity Transform)";
    }
    str << "\n";

    if (!DirectX::XMMatrixIsIdentity(trf))
        str << MathHelper::printMatrix(node->transform) << "\n";

    if (node->isBone)
    {
        str << "\nBone Offset:\n";
        str << MathHelper::printMatrix(node->boneOffset) << "\n";
    }

    for (UINT i = 0; i < node->children.size(); i++)
    {
        printNodes(str, node->children[i].get(), depth + 1);
    }

}


/*skinned model*/
struct SkinnedModel : Model
{
    /*gets copied to gpu*/ // belongs in model instance / game object
    std::vector<DirectX::XMFLOAT4X4> finalTransforms;

    /*bone information*/
    UINT boneCount = 0;
    std::vector<int> boneHierarchy;
    //std::vector<DirectX::XMFLOAT4X4> boneOffsets;
    Node nodeTree;
    DirectX::XMFLOAT4X4 globalInverse;

    void calculateFinalTransforms(AnimationClip* currentClip, float timePos);
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
    Sky, /*everything before transparent objects*/
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
    Material* MaterialOverwrite = nullptr;

    float animationTimer = 0.0f;

    bool isSkinned()
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

    Model* getModel()
    {
        return (isSkinned() ? skinnedModel : staticModel);
    }

    // Primitive topology.
    D3D12_PRIMITIVE_TOPOLOGY PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

    RenderType renderType = RenderType::Default;
    ShadowRenderType shadowType = ShadowRenderType::ShadowDefault;
};