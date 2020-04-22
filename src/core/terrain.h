#pragma once

#include "../util/d3dUtil.h"
#include "../render/renderresource.h"

class Terrain
{

public:

    explicit Terrain(const json& terrainInfo);

    void save();

    std::unique_ptr<Model> terrainModel = nullptr;

    float getHeight(float x, float z);

    void increaseHeight(float x, float z, float fallStart, float fallEnd, float increase);
    void paint(float x, float z, float fallStart, float fallEnd, int indexTexture);

    const float terrainSize = 250.0f;
    const UINT terrainSlices = 500;
    float heightScale = 200.0f;

    float cellSpacing = 0.0f;

    std::array<std::string, 4> textureStrings;

private:

    const std::string terrainPath = "data/level/";
    std::string terrainHeightMapFile;
    std::string terrainBlendMapFile;

    std::vector<float> mHeightMap;
    std::vector<DirectX::XMFLOAT4> mBlendMap;
    std::vector<TerrainVertex> mTerrainVertices;

    CD3DX12_GPU_DESCRIPTOR_HANDLE blendTexturesHandle[4];

    UINT objectCBSize = 0;
};