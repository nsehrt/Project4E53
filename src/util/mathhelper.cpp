#include "mathhelper.h"
#include <float.h>
#include <iostream>
#include <iomanip>
#include "serviceprovider.h"

using namespace DirectX;

const float MathHelper::Infinity = FLT_MAX;
const float MathHelper::Pi = XM_PI;
const float MathHelper::Epsilon = 0.01f;

float MathHelper::angleFromVector2(const DirectX::XMFLOAT2& vector)
{
    return std::atan2f(vector.y, vector.x);
}

float MathHelper::angleFromVector2Centered(const DirectX::XMFLOAT2& vector)
{
    if(vector.x == 0.0f && vector.y == 0.0f)
    {
        return 0.0f;
    }

    return angleFromVector2(vector) +DirectX::XM_PIDIV2;
}

DirectX::XMFLOAT3 MathHelper::vector3FromAngle(float angle)
{
    DirectX::XMFLOAT3 vec;

    vec.x = cos(angle);
    vec.y = 0.0f;
    vec.z = -sin(angle);

    return vec;
}

DirectX::XMFLOAT2 MathHelper::vector2FromAngleCentered(float angle)
{
    DirectX::XMFLOAT2 res{};

    float v = angle - DirectX::XM_PIDIV2;

    res.x = std::cosf(v);
    res.y = std::sinf(v);

    return res;
}

DirectX::XMFLOAT2 MathHelper::rotateUnitVectorByAngle(DirectX::XMFLOAT2& vec, float angle)
{
    DirectX::XMFLOAT2 res{};

    DirectX::XMStoreFloat2(&res, DirectX::XMVector2Transform(XMLoadFloat2(&vec), DirectX::XMMatrixRotationZ(angle)));
    DirectX::XMStoreFloat2(&res, DirectX::XMVector2Normalize(XMLoadFloat2(&res)));

    return res;
}

DirectX::XMVECTOR MathHelper::sphericalToCartesian(float radius, float theta, float phi)
{
    return DirectX::XMVectorSet(
        radius * std::sinf(phi) * std::cosf(theta),
        radius * std::cosf(phi),
        radius * std::sinf(phi) * std::sinf(theta),
        1.0f);
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