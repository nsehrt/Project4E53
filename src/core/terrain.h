#pragma once

#include "../util/d3dUtil.h"
#include "../render/renderresource.h"

class Terrain
{

public:

    explicit Terrain(const std::string& heightMap);

    void save();

    std::unique_ptr<Model> terrainModel = nullptr;

    float getHeight(float x, float z);

    void increaseHeight(float x, float z, float fallStart, float fallEnd, float increase);

    const float terrainSize = 250.0f;
    const UINT terrainSlices = 500;
    const float heightScale = 200.0f;

    float cellSpacing = 0.0f;

private:

    const std::string terrainPath = "data/level/";
    std::string terrainFile;

    std::vector<float> mHeightMap;
    std::vector<Vertex> mTerrainVertices;

    UINT objectCBSize = 0;
};