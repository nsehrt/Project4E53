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
    friend class P_4E53;
    friend class EditModeHUD;

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
    ObjectMotionType motionType = ObjectMotionType::Static;

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

    /*bullet physics*/
    float mass = 0.0f;
    float restitution = 0.0f;
    float damping = 0.0f;
    float friction = 0.5f;
    int shapeType = BOX_SHAPE_PROXYTYPE;
    int numericalID = -1; // unique int identifier

    public:
    DirectX::XMFLOAT3 extents = { 1,1,1 };

    protected:

    /*
    Shape types
    
    BOX_SHAPE_PROXYTYPE = 0
    SPHERE_SHAPE_PROXYTYPE = 8
    CAPSULE_SHAPE_PROXYTYPE = 10
    CYLINDER_SHAPE_PROXYTYPE = 13
    TERRAIN_SHAPE_PROXYTYPE = 24
    */

    btRigidBody* bulletBody = nullptr;

    /*transforms*/
    DirectX::XMFLOAT3 Position{}, Rotation{}, Scale{};
    DirectX::XMFLOAT3 TextureTranslation, TextureRotation, TextureScale;
    DirectX::XMFLOAT4X4 rotationQuat;

    /*render related*/
    bool currentlyInFrustum = false;
    UINT objectCBSize = 0;
    UINT skinnedCBSize = 0;


public:

    int getShape() const
    {
        return shapeType;
    }

    void setShape(int shape)
    {
        shapeType = shape;
    }

    /*Transform getter/setter*/
    void setPosition(DirectX::XMFLOAT3 _pos, bool updTrf = true)
    {
        Position = _pos;

        if(updTrf)
            updateTransforms();
    }

    void setScale(DirectX::XMFLOAT3 _scale);

    void setRotation(DirectX::XMFLOAT3 _rot, bool updTrf = true)
    {
        Rotation = _rot;
        DirectX::XMStoreFloat4x4(&rotationQuat,
                                 DirectX::XMMatrixRotationRollPitchYawFromVector(DirectX::XMLoadFloat3(&Rotation)));

        if(updTrf)
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