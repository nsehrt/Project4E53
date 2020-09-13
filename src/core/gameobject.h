#pragma once

#include "../render/renderresource.h"
#include "../core/basecollider.h"
#include <btBulletDynamicsCommon.h>

using json = nlohmann::json;

enum class ObjectType
{
    Default,
    Skinned,
    Wall,
    Sky,
    Debug,
    Terrain,
    Water,
    Grass,
    Particle
};

enum class ObjectMotionType
{
    Static,
    Kinetic,
    Dynamic
};

class GameObject
{

public:

    friend class BulletPhysics;

    /*load a game object from a json*/
    explicit GameObject(const json& objectJson, int index, int skinnedIndex = -1);

    /*empty game object without object cb, needed e.g. sky sphere*/
    explicit GameObject();

    /*create empty game object with object cb and skinned cb index*/
    explicit GameObject(const std::string& name, int index, int skinnedIndex = -1);

    ~GameObject();



    virtual void update(const GameTime& gt);
    bool draw() const;
    bool drawShadow() const;
    void drawPickBox() const;

    json toJson() const;


    ObjectType gameObjectType = ObjectType::Default;

    std::string Name;
    std::unique_ptr<RenderItem> renderItem = nullptr;
    float animationTimeScale = 1.0f;


    /*flags*/
    bool isCollisionEnabled = true;
    bool isDrawEnabled = true;
    bool isShadowEnabled = true;
    bool isFrustumCulled = true;
    bool isShadowForced = false;
    bool isSelectable = true;

    bool currentlyInShadowSphere = false;

    protected:

    BaseCollider collider;

    /*transforms*/
    DirectX::XMFLOAT3 Position, Rotation, Scale;
    DirectX::XMFLOAT3 TextureTranslation, TextureRotation, TextureScale;

    /*bullet physics*/
    ObjectMotionType motionType = ObjectMotionType::Static;
    float mass = 0.0f;
    int shapeType = 0;
    int numericalID = -1; // unique int identifier
    btRigidBody* bulletBody = nullptr;


    /*render related*/
    bool currentlyInFrustum = false;
    UINT objectCBSize = 0;
    UINT skinnedCBSize = 0;


    public:

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

    void setColliderProperties(BaseCollider::GameObjectCollider type, DirectX::XMFLOAT3 center, DirectX::XMFLOAT3 extents);
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

    BaseCollider& getCollider()
    {
        return collider;
    }

    void updateTransforms();

    bool exists(const nlohmann::json& j, const std::string& key)
    {
        return j.find(key) != j.end();
    }
};