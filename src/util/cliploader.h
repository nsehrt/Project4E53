#pragma once

#include "../render/renderresource.h"
#include "serviceprovider.h"
#include <filesystem>

class ClipLoader
{
public:
    explicit ClipLoader() = default;
    ~ClipLoader() = default;

    std::unique_ptr<AnimationClip> loadCLP(const std::filesystem::directory_entry& fileName);

private:

};