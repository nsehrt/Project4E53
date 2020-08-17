#pragma once

#include "../render/renderresource.h"
#include "../core/gamecollider.h"

using json = nlohmann::json;

enum class GameObjectType
{
    Static,
    Dynamic,
    Wall,
    Sky,
    Debug,
    Terrain,
    Water,
    Grass,
    Particle
};


class GameObject
{

public:

    /*load a game object from a json*/
    explicit GameObject(const json& objectJson, int index, int skinnedIndex = -1);

    /*empty game object without object cb, needed e.g. sky sphere*/
    explicit GameObject();

    /*create empty game object with object cb and skinned cb index*/
    explicit GameObject(const std::string& name, int index, int skinnedIndex = -1);

    ~GameObject() = default;



    virtual void update(const GameTime& gt);
    bool draw() const;
    bool drawShadow() const;
    void drawPickBox() const;

    json toJson() const;


    GameObjectType gameObjectType = GameObjectType::Static;
    std::string Name;
    std::unique_ptr<RenderItem> renderItem = nullptr;
    float animationTimeScale = 1.0f;


    /*Transform getter/setter*/
    void setPosition(DirectX::XMFLOAT3 _pos)
    {
        Position = _pos;

        updateTransforms();
    }

    void setScale(DirectX::XMFLOAT3 _scale);

    void setRotation(DirectX::XMFLOAT3 _rot)
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
    void setTextureTranslation(DirectX::XMFLOAT3 _translation)
    {
        TextureTranslation = _translation;

        updateTransforms();
    }

    void setTextureScale(DirectX::XMFLOAT3 _scale)
    {
        TextureScale = _scale;

        updateTransforms();
    }

    void setTextureRotation(DirectX::XMFLOAT3 _rot)
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

    void setColliderProperties(GameCollider::GameObjectCollider type, DirectX::XMFLOAT3 center, DirectX::XMFLOAT3 extents);
    bool intersects(const GameObject& obj) const;

    bool getIsInFrustum() const
    {
        return currentlyInFrustum;
    }

    void makeDynamic(SkinnedModel* sModel, UINT skinnedCBIndex);
    void setAnimation(AnimationClip* aClip, bool keepRelativeTime = false);

    void checkInViewFrustum(DirectX::BoundingFrustum& localCamFrustum);
    void resetInViewFrustum()
    {
        currentlyInFrustum = false;
        currentlyInShadowSphere = false;
    }

    GameCollider& getCollider()
    {
        return collider;
    }

    void updateTransforms();

    /*flags*/
    bool isCollisionEnabled = true;
    bool isDrawEnabled = true;
    bool isShadowEnabled = true;
    bool isFrustumCulled = true;
    bool isShadowForced = false;
    bool isSelectable = true;

    bool currentlyInShadowSphere = false;

protected:

    GameCollider collider;

    /*transforms*/
    DirectX::XMFLOAT3 Position, Rotation, Scale;
    DirectX::XMFLOAT3 TextureTranslation, TextureRotation, TextureScale;

    
    bool currentlyInFrustum = false;

    UINT objectCBSize = 0;
    UINT skinnedCBSize = 0;

    bool exists(const nlohmann::json& j, const std::string& key)
    {
        return j.find(key) != j.end();
    }
};