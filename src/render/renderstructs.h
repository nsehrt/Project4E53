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

    void interpolate(float time, DirectX::XMFLOAT4X4& matrix) const
    {
        if (time <= keyFrames.front().timeStamp)
        {
            DirectX::XMVECTOR S = DirectX::XMLoadFloat3(&keyFrames.front().scale);
            DirectX::XMVECTOR P = DirectX::XMLoadFloat3(&keyFrames.front().translation);
            DirectX::XMVECTOR Q = DirectX::XMLoadFloat4(&keyFrames.front().rotationQuat);

            DirectX::XMVECTOR zero = DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
            XMStoreFloat4x4(&matrix, DirectX::XMMatrixAffineTransformation(S, zero, Q, P));
        }
        else if (time >= keyFrames.back().timeStamp)
        {
            DirectX::XMVECTOR S = DirectX::XMLoadFloat3(&keyFrames.back().scale);
            DirectX::XMVECTOR P = DirectX::XMLoadFloat3(&keyFrames.back().translation);
            DirectX::XMVECTOR Q = DirectX::XMLoadFloat4(&keyFrames.back().rotationQuat);

            DirectX::XMVECTOR zero = DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
            DirectX::XMStoreFloat4x4(&matrix, DirectX::XMMatrixAffineTransformation(S, zero, Q, P));
        }
        else
        {
            for (UINT i = 0; i < keyFrames.size() - 1; ++i)
            {
                if (time >= keyFrames[i].timeStamp && time <= keyFrames[(INT_PTR)i + 1].timeStamp)
                {
                    float lerpPercent = (time - keyFrames[i].timeStamp) / (keyFrames[(INT_PTR)i + 1].timeStamp - keyFrames[i].timeStamp);

                    DirectX::XMVECTOR s0 = DirectX::XMLoadFloat3(&keyFrames[i].scale);
                    DirectX::XMVECTOR s1 = DirectX::XMLoadFloat3(&keyFrames[(INT_PTR)i + 1].scale);

                    DirectX::XMVECTOR p0 = DirectX::XMLoadFloat3(&keyFrames[i].translation);
                    DirectX::XMVECTOR p1 = DirectX::XMLoadFloat3(&keyFrames[(INT_PTR)i + 1].translation);

                    DirectX::XMVECTOR q0 = DirectX::XMLoadFloat4(&keyFrames[i].rotationQuat);
                    DirectX::XMVECTOR q1 = DirectX::XMLoadFloat4(&keyFrames[(INT_PTR)i + 1].rotationQuat);

                    DirectX::XMVECTOR S = DirectX::XMVectorLerp(s0, s1, lerpPercent);
                    DirectX::XMVECTOR P = DirectX::XMVectorLerp(p0, p1, lerpPercent);
                    DirectX::XMVECTOR Q = DirectX::XMQuaternionSlerp(q0, q1, lerpPercent);

                    DirectX::XMVECTOR zero = DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
                    DirectX::XMStoreFloat4x4(&matrix, DirectX::XMMatrixAffineTransformation(S, zero, Q, P));

                    break;
                }
            }
        }
    }

};


struct AnimationClip
{
    std::vector<BoneAnimation> boneAnimations;
    std::string name;

private:
    float startTime = -1.0f;
    float endTime = -1.0f;

public:

    float getStartTime()
    {
        if (startTime != -1.0f)
        {
            return startTime;
        }

        float result = MathHelper::Infinity;

        for (const auto& i : boneAnimations)
        {
            result = MathHelper::minH(result, i.getStartTime());
        }

        startTime = result;

        return result;
    }

    float getEndTime()
    {
        if (endTime != -1.0f)
        {
            return endTime;
        }

        float result = 0.0f;

        for (const auto& i : boneAnimations)
        {
            result = MathHelper::maxH(result, i.getEndTime());
        }

        endTime = result;

        return result;
    }



    void interpolate(float t, std::vector<DirectX::XMFLOAT4X4>& boneTransforms)const
    {
        for (UINT i = 0; i < boneTransforms.size(); ++i)
        {
            boneAnimations[i].interpolate(t, boneTransforms[i]);
        }
    }
};

/*skinned model*/
struct SkinnedModel : Model
{
    /*gets copied to gpu*/
    std::vector<DirectX::XMFLOAT4X4> finalTransforms;

    /*bone information*/
    UINT boneCount = 0;
    std::vector<int> boneHierarchy;
    std::vector<DirectX::XMFLOAT4X4> boneOffsets;


    void calculateFinalTransforms(AnimationClip* currentClip, float timePos)
    {
        UINT numBones = (UINT) boneOffsets.size();

        std::vector<DirectX::XMFLOAT4X4> toParentTransforms(numBones);

        currentClip->interpolate(timePos, toParentTransforms);

        std::vector<DirectX::XMFLOAT4X4> toRootTransforms(numBones);

        toRootTransforms[0] = toParentTransforms[0];

        // find the toRootTransform of the children.
        for (UINT i = 1; i < numBones; ++i)
        {
            DirectX::XMMATRIX toParent = DirectX::XMLoadFloat4x4(&toParentTransforms[i]);

            int parentIndex = boneHierarchy[i];
            DirectX::XMMATRIX parentToRoot = DirectX::XMLoadFloat4x4(&toRootTransforms[parentIndex]);

            DirectX::XMMATRIX toRoot = DirectX::XMMatrixMultiply(toParent, parentToRoot);

            XMStoreFloat4x4(&toRootTransforms[i], toRoot);
        }

        // Premultiply by the bone offset transform to get the final transform.
        for (UINT i = 0; i < numBones; ++i)
        {
            DirectX::XMMATRIX offset = DirectX::XMLoadFloat4x4(&boneOffsets[i]);
            DirectX::XMMATRIX toRoot = DirectX::XMLoadFloat4x4(&toRootTransforms[i]);
            DirectX::XMMATRIX finalTransform = DirectX::XMMatrixMultiply(offset, toRoot);
            DirectX::XMStoreFloat4x4(&finalTransforms[i], DirectX::XMMatrixTranspose(finalTransform));
        }
    }


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
    SkinnedBind,
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