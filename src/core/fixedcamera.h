#pragma once

#include "camera.h"
#include "../input/inputmanager.h"
#include "gametime.h"
#include "../util/serviceprovider.h"

class FixedCamera : public Camera
{
public:
    FixedCamera() = default;
    ~FixedCamera() = default;

    void updateFixedCamera(const DirectX::XMFLOAT3& targetPos, float zoomDelta, float turnDelta);
    void initFixedDistance(float minDistance, float maxDistance);
    float getDistanceNormalized()
    {
        return mCurrentDistance / mMaxDistance;
    }

    float getTurn()
    {
        return mTurn;
    }

private:

    float mCurrentDistance = 15.0f;
    float mMinDistance = 15.0f;
    float mMaxDistance = 100.0f;
    float mTurn = XM_PI;
};