#pragma once

#define TERRAIN_SIZE 1024
#define TERRAIN_SLICES 250

#define HEIGHTMAP_SIZE 2049
#define HEIGHTMAP_SCALE 50.0f

#include "../util/d3dUtil.h"
#define TERRAIN_PATH "data/level/"


class Terrain
{

public:

    explicit Terrain(const std::string& heightMap);

    void draw();

    std::unique_ptr<Model> terrainModel = nullptr;
    std::unique_ptr<RenderItem> terrainRItem = nullptr;

private:

    float calculateHeight(float x, float z) const;
    DirectX::XMFLOAT3 calculateNormal(float x, float z) const;

    std::vector<float> mHeightMap;

    float getWidth() const;
    float getDepth() const;
    float getHeight(float x, float y) const;

    UINT objectCBSize = 0;
};