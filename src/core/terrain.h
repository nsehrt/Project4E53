#pragma once

#define TERRAIN_SIZE 128
#define TERRAIN_SLICES 256

#include "../util/d3dUtil.h"

class Terrain
{

public:

    explicit Terrain(const std::string& heightMap);

    void draw();

private:

    float getWidth() const;
    float getDepth() const;
    float getHeight(float x, float y) const;



};