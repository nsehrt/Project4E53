#pragma once

#include "../extern/json.hpp"
#include "../render/renderstructs.h"
#include "../render/renderresource.h"
#include "../util/serviceprovider.h"

using json = nlohmann::json;

enum class GameObjectType
{
    Static,
    Wall,
    Sky,
    Dynamic,
    Debug,
    Terrain,
    Water,
    Grass
};

class GameObject
{
public:

    explicit GameObject(const json& objectJson, int index);

    explicit GameObject();

    explicit GameObject(int index);

    ~GameObject() = default;

    void update(const GameTime& gt);
    bool draw();
    bool drawShadow();
    void drawRoughHitbox();

    json toJson();

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

    float getRoughHitBoxExtentY()
    {
        return roughBoundingBox.Extents.y;
    }

    void setRoughHitBoxExtents(DirectX::XMFLOAT3 e)
    {
        roughBoundingBox.Extents = e;
    }

    DirectX::BoundingBox& getRoughBoundingBox()
    {
        return roughBoundingBox;
    }

    bool getIsInFrustum()
    {
        return isInFrustum;
    }

    void checkInViewFrustum();

    bool intersectsRough(GameObject& obj);
    bool intersectsRough(DirectX::BoundingBox& box);

    bool intersectsShadowBounds(DirectX::BoundingSphere& sphere); /*only for shadow culling*/

    void updateTransforms();

    /*flags*/
    bool isCollisionEnabled = true;
    bool isDrawEnabled = true;
    bool isShadowEnabled = true;
    bool isShadowForced = false;
    bool isFrustumCulled = true;

private:

    /*transforms*/
    DirectX::XMFLOAT3 Position, Rotation, Scale;

    DirectX::XMFLOAT3 TextureTranslation, TextureRotation, TextureScale;

    /*rough hitbox*/
    DirectX::BoundingBox roughBoundingBox;

    bool isInFrustum = false;

    /*precise hitbox needed*/

    UINT objectCBSize = 0;

    bool exists(const nlohmann::json& j, const std::string& key)
    {
        return j.find(key) != j.end();
    }
};