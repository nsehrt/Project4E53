#include "fixedcamera.h"

using namespace DirectX;

void FixedCamera::updateFixedCamera(const DirectX::XMFLOAT3& targetPos, float zoomDelta, float turnDelta)
{
    /*calculate zoom level*/
    mCurrentDistance = MathHelper::clampH(mCurrentDistance + zoomDelta, mMinDistance, mMaxDistance);

    mTurn += turnDelta;
    if (mTurn > XM_2PI) mTurn -= XM_2PI;

    XMFLOAT3 newPos = targetPos;

    newPos.y += mCurrentDistance;
    newPos.x += sin(mTurn) * mCurrentDistance;
    newPos.z += cos(mTurn) * mCurrentDistance;

    mTarget = targetPos;
    lookAt(newPos, targetPos, mUpConst);
}

void FixedCamera::initFixedDistance(float minDistance, float maxDistance)
{
    mMinDistance = minDistance;
    mMaxDistance = maxDistance;
    mCurrentDistance = (maxDistance - minDistance) * 0.15f + minDistance;
}