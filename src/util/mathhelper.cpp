#include "mathhelper.h"
#include <float.h>
#include <iostream>
#include <iomanip>
#include "serviceprovider.h"

using namespace DirectX;

const float MathHelper::Infinity = FLT_MAX;
const float MathHelper::Pi = XM_PI;
const float MathHelper::Epsilon = 0.01f;

float MathHelper::angleFromXY(float x, float y)
{
    float theta = 0.0f;

    if(x == 0.0f && y == 0.0f)
    {
        return Pi;
    }

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

DirectX::XMFLOAT2 MathHelper::rotateByAngle(DirectX::XMFLOAT2& vec, float angle)
{
    DirectX::XMFLOAT2 res{};

    res.x = std::cosf(angle * vec.x) - std::sinf(angle * vec.y);
    res.y = std::sinf(angle * vec.x) + std::cosf(angle * vec.y);

    return res;
}

std::string MathHelper::printMatrix(const DirectX::XMFLOAT4X4& m, bool toLog)
{
    std::stringstream ret;

    ret << std::fixed << std::setprecision(2) << "\n" << m(0, 0) << " | " << m(0, 1) << " | " << m(0, 2) << " | " << m(0, 3) << "\n"
        << m(1, 0) << " | " << m(1, 1) << " | " << m(1, 2) << " | " << m(1, 3) << "\n"
        << m(2, 0) << " | " << m(2, 1) << " | " << m(2, 2) << " | " << m(2, 3) << "\n"
        << m(3, 0) << " | " << m(3, 1) << " | " << m(3, 2) << " | " << m(3, 3) << "\n";

    if (toLog)
    {
        LOG(Severity::Debug, ret.str());
    }


    return ret.str();
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