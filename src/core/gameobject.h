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

        TextureTranslation = XMFLOAT3(0.0f, 0.0f, 0.0f);
        TextureRotation = XMFLOAT3(0.0f, 0.0f, 0.0f);
        TextureScale = XMFLOAT3(0.0f, 0.0f, 0.0f);
    };
    ~GameObject() = default;


    void update(const GameTime &gt);
    void draw(RenderType _renderType, ID3D12Resource* objectCB);
    void drawHitbox(RenderType _renderType, ID3D12Resource* objectCB);

    std::string name;
    std::unique_ptr<RenderItem> renderItem;
    GameObjectType gameObjectType = GameObjectType::Static;


    /*Transform getter/setter*/
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

    /*Texture transform getter/setter*/
    void setTextureTranslation(XMFLOAT3 _translation)
    {
        TextureTranslation = _translation;

        updateTransforms();
    }

    void setTextureScale(XMFLOAT3 _scale)
    {
        TextureScale = _scale;

        updateTransforms();
    }

    void setTextureRotation(XMFLOAT3 _rot)
    {
        TextureRotation = _rot;

        updateTransforms();
    }

    DirectX::XMFLOAT3 getTextureTranslation() const
    {
        return TextureTranslation;
    }

    DirectX::XMFLOAT3 getTextureScale() const
    {
        return TextureScale;
    }

    DirectX::XMFLOAT3 getTextureRotation() const
    {
        return TextureRotation;
    }


    bool intersects(GameObject& obj);
    bool intersects(DirectX::BoundingBox& box);

    void updateTransforms();

    /*flags*/
    bool isCollisionEnabled = true;
    bool isDrawEnabled = true;
    bool isShadowEnabled = true;

private:

    /*transforms*/
    DirectX::XMFLOAT3 Position, Rotation, Scale;

    DirectX::XMFLOAT3 TextureTranslation, TextureRotation, TextureScale;

    /*hitbox*/
    DirectX::BoundingBox hitBox;


    UINT objectCBSize = 0;

    bool exists(const nlohmann::json& j, const std::string& key)
    {
        return j.find(key) != j.end();
    }

    friend class RenderResource;

};