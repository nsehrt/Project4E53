#pragma once

#include "../extern/json.hpp"
#include "../render/renderstructs.h"
#include "../render/renderresource.h"
#include "../util/serviceprovider.h"

using json = nlohmann::json;

enum class GameObjectType
{
    Static,
    Sky,
    Dynamic
};

class GameObject
{
public:

    explicit GameObject(const json& objectJson, int index);
    GameObject()
    {
        Position = XMFLOAT3(0.0f, 0.0f, 0.0f);
        Rotation = XMFLOAT3(0.0f, 0.0f, 0.0f);
        Scale = XMFLOAT3(0.0f, 0.0f, 0.0f);
    };
    ~GameObject() = default;


    void update(const GameTime &gt);
    void draw(RenderType _renderType, ID3D12Resource* objectCB);
    void drawHitbox(RenderType _renderType, ID3D12Resource* objectCB);

    std::string name;
    std::unique_ptr<RenderItem> renderItem;
    GameObjectType gameObjectType = GameObjectType::Static;

    void setPosition(XMFLOAT3 _pos)
    {
        Position = _pos;
        
        updateTransforms();
    }

    void setScale(XMFLOAT3 _scale)
    {
        Scale = _scale;

        updateTransforms();
    }

    void setRotation(XMFLOAT3 _rot)
    {
        Rotation = _rot;

        updateTransforms();
    }

    DirectX::XMFLOAT3 getPosition() const
    {
        return Position;
    }

    DirectX::XMFLOAT3 getScale() const
    {
        return Scale;
    }

    DirectX::XMFLOAT3 getRotation() const
    {
        return Rotation;
    }

    void updateTransforms();

    /*flags*/
    bool isCollisionEnabled = true;
    bool isDrawEnabled = true;
    bool isShadowEnabled = true;


    DirectX::BoundingBox hitBox;

private:

    /*transforms*/
    DirectX::XMFLOAT3 Position, Rotation, Scale;

    UINT objectCBSize = 0;
    friend class RenderResource;

};