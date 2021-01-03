#pragma once

#include "../util/d3dUtil.h"

/*this class holds only the basic AABB and OBB, everything else is handled by bullet*/

class BaseCollider
{

public:

    explicit BaseCollider();
    ~BaseCollider() = default;

    void setBaseBoxes(DirectX::BoundingBox box);
    void update(const DirectX::XMFLOAT4X4& M);

    DirectX::BoundingBox& getFrustumBox()
    {
        return frustumCheckBoundingBox;
    }

    DirectX::BoundingOrientedBox& getPickBox()
    {
        return pickBox;
    }

    DirectX::XMFLOAT3 getInternalPickBoxOffset() const
    {
        return internalPickBox.Center;
    }

    /*collision functions*/
    bool intersects(const DirectX::BoundingFrustum& cameraFrustum) const;
    bool intersects(const DirectX::BoundingSphere& shadowSphere) const;

private:

    /*is always baseModelBox * world*/
    DirectX::BoundingBox internalFrustumCheckBoundingBox;
    DirectX::BoundingBox frustumCheckBoundingBox;

    DirectX::BoundingOrientedBox internalPickBox;
    DirectX::BoundingOrientedBox pickBox;
};