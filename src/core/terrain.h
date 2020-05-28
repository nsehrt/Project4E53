#pragma once

#include "../util/d3dUtil.h"
#include "../render/renderresource.h"

class Terrain
{
public:

    explicit Terrain(const json& terrainInfo);

    bool save();

    std::unique_ptr<Model> terrainModel = nullptr;

    float getHeight(float x, float z);

    void increaseHeight(float x, float z, float fallStart, float fallEnd, float increase, float resetHeight, bool setHeight, bool setZero);
    void paint(float x, float z, float fallStart, float fallEnd, float increase, int indexTexture, bool setZero = false);

    const float terrainSize = 150.0f;
    const UINT terrainSlices = 500;
    float heightScale = 150.0f;

    float cellSpacing = 0.0f;

    std::array<std::string, 4> textureStrings;

    CD3DX12_GPU_DESCRIPTOR_HANDLE blendTexturesHandle[4];

    std::string terrainHeightMapFileStem;
    std::string terrainBlendMapFileStem;

private:

    bool saveBlendMap();
    bool saveHeightMap();

    const std::string terrainPath = "data/level/";

    std::string terrainHeightMapFile;
    std::string terrainBlendMapFile;

    std::vector<float> mHeightMap;
    std::vector<DirectX::XMFLOAT4> mBlendMap;
    std::vector<TerrainVertex> mTerrainVertices;
    Microsoft::WRL::ComPtr<ID3D12Resource> holder;

    UINT objectCBSize = 0;
};