#pragma once

#include "../extern/json.hpp"
#include "../render/renderstructs.h"
#include "../render/renderresource.h"
#include "../util/serviceprovider.h"

using json = nlohmann::json;

class GameObject
{
public:

    explicit GameObject(const json& objectJson, int index);
    explicit GameObject() = default;
    ~GameObject() = default;


    std::string name;
    std::unique_ptr<RenderItem> renderItem;

    /*transforms*/
    DirectX::XMFLOAT3 Position, Rotation, Scale;

    /*flags*/
    bool isCollisionEnabled = true;
    bool isDrawEnabled = true;
    bool isShadowEnabled = true;


    DirectX::BoundingOrientedBox hitBox;

private:


    friend class RenderResource;

};