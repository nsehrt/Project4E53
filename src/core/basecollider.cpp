#include "../core/basecollider.h"

using namespace DirectX;

BaseCollider::BaseCollider()
{
    internalFrustumCheckBoundingBox = BoundingBox({ 0,0,0 }, { 0.5f,0.5f,0.5f });
    internalPickBox = BoundingOrientedBox({ 0,0,0 }, { 0.5f,0.5f,0.5f }, { 0.0f,0.0f,0.0f,1.0f });
}

/*set the standard boxes for frustum checking and picking*/
void BaseCollider::setBaseBoxes(DirectX::BoundingBox box)
{
    internalFrustumCheckBoundingBox = box;
    internalPickBox = BoundingOrientedBox(box.Center, box.Extents, { 0,0,0,1 });

}


/*update all bounding boxes with world transform*/
void BaseCollider::update(const DirectX::XMFLOAT4X4& M)
{
    internalFrustumCheckBoundingBox.Transform(frustumCheckBoundingBox, XMLoadFloat4x4(&M));
    internalPickBox.Transform(pickBox, XMLoadFloat4x4(&M));
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
