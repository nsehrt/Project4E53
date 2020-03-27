#pragma once

#include "../render/renderstructs.h"

class ModelLoader
{
public:
    ModelLoader() = default;
    ~ModelLoader() = default;

    static bool loadB3D(const std::string& fileName, Mesh* m);
};