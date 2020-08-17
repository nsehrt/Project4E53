#pragma once

#include "../util/d3dUtil.h"

class GameCollider
{

public:
    enum class GameObjectCollider
    {
        OBB,
        Sphere
    };

    explicit GameCollider();
    ~GameCollider() = default;

    void setBaseBoxes(DirectX::BoundingBox box);
    void setColliderType(GameObjectCollider goCollider);
    void setProperties(DirectX::XMFLOAT3 center, DirectX::XMFLOAT3 extents);

    void update(const DirectX::XMFLOAT4X4& M);



    DirectX::BoundingBox& getFrustumBox()
    {
        return frustumCheckBoundingBox;
    }

    DirectX::BoundingOrientedBox& getPickBox()
    {
        return pickBox;
    }

    GameObjectCollider getType() const
    {
        return colliderType;
    }

    DirectX::XMFLOAT3 getRelativeCenterOffset() const;
    DirectX::XMFLOAT3 getCenterOffset() const;
    DirectX::XMFLOAT3 getExtents() const;
    DirectX::XMFLOAT3 getInternalPickBoxOffset() const
    {
        return internalPickBox.Center;
    }

    /*collision functions*/
    bool intersects(const DirectX::BoundingFrustum& cameraFrustum) const;
    bool intersects(const GameCollider& other) const;
    bool intersects(const DirectX::BoundingSphere& shadowSphere) const;

private:

    GameObjectCollider colliderType = GameObjectCollider::OBB;

    /*is always baseModelBox * world*/
    DirectX::BoundingBox internalFrustumCheckBoundingBox;
    DirectX::BoundingBox frustumCheckBoundingBox;

    DirectX::BoundingOrientedBox internalPickBox;
    DirectX::BoundingOrientedBox pickBox;

    DirectX::BoundingOrientedBox internalBoundingBox;
    DirectX::BoundingSphere internalBoundingSphere;

    DirectX::BoundingOrientedBox boundingBox;
    DirectX::BoundingSphere boundingSphere;
};