#include "fixedcamera.h"
#include "../util/mathhelper.h"

void FixedCamera::updateFixedCamera(const DirectX::XMFLOAT3& targetPos, float zoomDelta)
{
    /*calculate zoom level*/
    mCurrentDistance = MathHelper::clampH(mCurrentDistance + zoomDelta, mMinDistance, mMaxDistance);

    XMFLOAT3 newPos = targetPos;

    XMFLOAT3 mAdd = XMFLOAT3(0.0f, mCurrentDistance, -mCurrentDistance);
    XMVECTOR n = XMLoadFloat3(&newPos);
    n = XMVectorAdd(n, XMLoadFloat3(&mAdd));
    XMStoreFloat3(&newPos, n);

    lookAt(newPos, targetPos, mUp);
}

void FixedCamera::initFixedDistance(float minDistance, float maxDistance)
{
    mMinDistance = minDistance;
    mMaxDistance = maxDistance;
    mCurrentDistance = (maxDistance - minDistance) * 0.15f + minDistance;
}