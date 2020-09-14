#include "../core/basecollider.h"

using namespace DirectX;

BaseCollider::BaseCollider()
{
    internalFrustumCheckBoundingBox = BoundingBox({ 0,0,0 }, { 0.5f,0.5f,0.5f });
    internalBoundingBox = BoundingOrientedBox({ 0,0,0 }, { 0.5f,0.5f,0.5f }, { 0.0f,0.0f,0.0f,1.0f });
    internalPickBox = internalBoundingBox;
    internalBoundingSphere = BoundingSphere({ 0,0,0 }, 0.5f);
}

/*set the standard boxes for frustum checking and picking*/
void BaseCollider::setBaseBoxes(DirectX::BoundingBox box)
{
    setColliderType(GameObjectCollider::OBB);

    internalFrustumCheckBoundingBox = box;
    internalPickBox = BoundingOrientedBox(box.Center, box.Extents, { 0,0,0,1 });

    setProperties(box.Center, box.Extents);
}

/*change between gameobjectcollider types*/
void BaseCollider::setColliderType(GameObjectCollider goCollider)
{
    colliderType = goCollider;
}

/*change size and location of the actual bounding boxes*/
void BaseCollider::setProperties(DirectX::XMFLOAT3 center, DirectX::XMFLOAT3 extents)
{
    if (colliderType == GameObjectCollider::OBB)
    {
        internalBoundingBox = BoundingOrientedBox(center, extents, { 0,0,0,1 });
    }
    else if(colliderType == GameObjectCollider::Sphere)
    {
        internalBoundingSphere = BoundingSphere(center, extents.x);
    }

}

/*update all bounding boxes with world transform*/
void BaseCollider::update(const DirectX::XMFLOAT4X4& M)
{
    internalFrustumCheckBoundingBox.Transform(frustumCheckBoundingBox, XMLoadFloat4x4(&M));
    internalPickBox.Transform(pickBox, XMLoadFloat4x4(&M));

    internalBoundingBox.Transform(boundingBox, XMLoadFloat4x4(&M));
    internalBoundingSphere.Transform(boundingSphere, XMLoadFloat4x4(&M));

    boundingBox.Extents = internalBoundingBox.Extents;
    boundingSphere.Radius = internalBoundingSphere.Radius;
}

DirectX::XMFLOAT3 BaseCollider::getRelativeCenterOffset() const
{
    if (colliderType == GameObjectCollider::OBB)
    {
        return internalBoundingBox.Center;
    }
    else if (colliderType == GameObjectCollider::Sphere)
    {
        return internalBoundingSphere.Center;
    }

    return{};
}

DirectX::XMFLOAT3 BaseCollider::getCenterOffset() const
{
    if(colliderType == GameObjectCollider::OBB)
    {
        return boundingBox.Center;
    }
    else if(colliderType == GameObjectCollider::Sphere)
    {
        return boundingSphere.Center;
    }

    return{};
}

DirectX::XMFLOAT3 BaseCollider::getExtents() const
{
    if (colliderType == GameObjectCollider::OBB)
    {
        return internalBoundingBox.Extents;
    }
    else if (colliderType == GameObjectCollider::Sphere)
    {
        return { internalBoundingSphere.Radius, internalBoundingSphere.Radius, internalBoundingSphere.Radius };
    }

    return {};
}

/*camera frustum check*/
bool BaseCollider::intersects(const DirectX::BoundingFrustum& cameraFrustum) const
{
    return cameraFrustum.Contains(frustumCheckBoundingBox) == DirectX::DISJOINT;
}

/*intersect with shadow sphere*/
bool BaseCollider::intersects(const DirectX::BoundingSphere& shadowSphere) const
{
    return frustumCheckBoundingBox.Intersects(shadowSphere);
}
