#include "gamecollider.h"

using namespace DirectX;

GameCollider::GameCollider()
{
    internalFrustumCheckBoundingBox = BoundingBox({ 0,0,0 }, { 0.5f,0.5f,0.5f });
    internalBoundingBox = BoundingOrientedBox({ 0,0,0 }, { 0.5f,0.5f,0.5f }, { 0.0f,0.0f,0.0f,1.0f });
    internalPickBox = internalBoundingBox;
    internalBoundingSphere = BoundingSphere({ 0,0,0 }, 0.5f);
}

/*set the standard boxes for frustum checking and picking*/
void GameCollider::setBaseBoxes(DirectX::BoundingBox box)
{
    setColliderType(GameObjectCollider::OBB);

    internalFrustumCheckBoundingBox = box;
    internalPickBox = BoundingOrientedBox(box.Center, box.Extents, { 0,0,0,1 });

    setProperties(box.Center, box.Extents);
}

/*change between gameobjectcollider types*/
void GameCollider::setColliderType(GameObjectCollider goCollider)
{
    colliderType = goCollider;
}

/*change size and location of the actual bounding boxes*/
void GameCollider::setProperties(DirectX::XMFLOAT3 center, DirectX::XMFLOAT3 extents)
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
void GameCollider::update(const DirectX::XMFLOAT4X4& M)
{
    internalBoundingBox.Transform(boundingBox, XMLoadFloat4x4(&M));
    internalPickBox.Transform(pickBox, XMLoadFloat4x4(&M));
    internalBoundingSphere.Transform(boundingSphere, XMLoadFloat4x4(&M));
    internalFrustumCheckBoundingBox.Transform(frustumCheckBoundingBox, XMLoadFloat4x4(&M));
}

/*camera frustum check*/
bool GameCollider::intersects(const DirectX::BoundingFrustum& cameraFrustum) const
{
    return cameraFrustum.Contains(frustumCheckBoundingBox) == DirectX::DISJOINT;
}

/*intersect with other game collider*/
bool GameCollider::intersects(const GameCollider& other) const
{
    if (colliderType == GameObjectCollider::OBB && other.colliderType == GameObjectCollider::OBB)
    {
        return boundingBox.Intersects(other.boundingBox);
    }
    else if (colliderType == GameObjectCollider::OBB && other.colliderType == GameObjectCollider::Sphere)
    {
        return boundingBox.Intersects(other.boundingSphere);
    }
    else if (colliderType == GameObjectCollider::Sphere && other.colliderType == GameObjectCollider::OBB)
    {
        return boundingSphere.Intersects(other.boundingBox);
    }
    else
    {
        return boundingSphere.Intersects(other.boundingSphere);
    }
}

/*intersect with shadow sphere*/
bool GameCollider::intersects(const DirectX::BoundingSphere& shadowSphere) const
{
    return frustumCheckBoundingBox.Intersects(shadowSphere);
}
