#include "mathhelper.h"
#include <float.h>
#include <cmath>

using namespace DirectX;

const float MathHelper::Infinity = FLT_MAX;
const float MathHelper::Pi = XM_PI;

float MathHelper::angleFromXY(float x, float y)
{
    float theta = 0.0f;

    if (x >= 0.0f)
    {
        theta = atanf(y / x);
        if (theta < 0.0f)
            theta += 2.0f * Pi;
    }
    else
    {
        theta = atanf(y / x) + Pi;
    }

    return theta;
}

/*return random unit length vector3*/
XMVECTOR MathHelper::randUnitVec3()
{
    XMVECTOR One = XMVectorSet(1.0f, 1.0f, 1.0f, 1.0f);
    XMVECTOR Zero = XMVectorZero();

    while (true)
    {
        XMVECTOR v = XMVectorSet(MathHelper::randF(-1.0, 1.0f), MathHelper::randF(-1.0, 1.0f), MathHelper::randF(-1.0, 1.0f), 0.0f);

        if (XMVector3Greater(XMVector3LengthSq(v), One))
            continue;

        return XMVector3Normalize(v);
    }
}

XMVECTOR MathHelper::randHemisphereUnitVec3(XMVECTOR n)
{
    XMVECTOR One = XMVectorSet(1.0f, 1.0f, 1.0f, 1.0f);
    XMVECTOR Zero = XMVectorZero();

    while (true)
    {
        XMVECTOR v = XMVectorSet(MathHelper::randF(-1.0f, 1.0f), MathHelper::randF(-1.0f, 1.0f), MathHelper::randF(-1.0f, 1.0f), 0.0f);

        if (XMVector3Greater(XMVector3LengthSq(v), One))
            continue;

        if (XMVector3Less(XMVector3Dot(n, v), Zero))
            continue;

        return XMVector3Normalize(v);
    }
}