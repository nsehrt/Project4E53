#pragma once

#include "../util/d3dUtil.h"


class Terrain
{

public:

    explicit Terrain(const std::string& heightMap);

    void save();

    std::unique_ptr<Model> terrainModel = nullptr;
    std::unique_ptr<RenderItem> terrainRItem = nullptr;

    const UINT terrainSize = 1000;
    const UINT terrainSlices = 500;
    const float heightScale = 100.0f;

private:

    const std::string terrainPath = "data/level/";
    std::string terrainFile;

    float calculateHeight(float x, float z) const;
    DirectX::XMFLOAT3 calculateNormal(float x, float z) const;

    
    std::vector<float> mHeightMap;


    float getWidth() const;
    float getDepth() const;
    float getHeight(float x, float y) const;

    UINT objectCBSize = 0;
};