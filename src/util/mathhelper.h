#pragma once

#include <Windows.h>
#include <DirectXMath.h>
#include <cmath>
#include <cstdint>
#include <iostream>

class MathHelper
{
public:
    /*random float between 0.0 and <1.0*/
    static float randF()
    {
        return (float)(rand()) / (FLOAT)RAND_MAX;
    }

    /*random float between a and <b*/
    static float randF(float a, float b)
    {
        return a + randF() * (b - a);
    }

    /*random int between a and b*/
    static int randI(int a, int b)
    {
        return a + rand() % ((b - a) + 1);
    }

    /*min*/
    template<typename T>
    static T minH(const T& a, const T& b)
    {
        return a < b ? a : b;
    }

    /*max*/
    template<typename T>
    static T maxH(const T& a, const T& b)
    {
        return a > b ? a : b;
    }

    /*lerp*/
    template<typename T>
    static T lerpH(const T& a, const T& b, float t)
    {
        return a + (b - a) * t;
    }

    /*clamp*/
    template<typename T>
    static T clampH(const T& x, const T& low, const T& high)
    {
        return x < low ? low : (x > high ? high : x);
    }

    /*smoothstep*/
    static float smoothStepH(const float edge0, const float edge1, const float value)
    {
        float result = clampH((value - edge0) / (edge1 - edge0), 0.0f, 1.0f);
        return result * result * (3 - 2 * result);
    }

    /*map*/
    template<typename T>
    static T mapH(T x, T in_min, T in_max, T out_min, T out_max)
    {
        return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
    }

    /*polar angle of point*/
    static float angleFromXY(float x, float y);

    static DirectX::XMVECTOR sphericalToCartesian(float radius, float theta, float phi)
    {
        return DirectX::XMVectorSet(
            radius * std::sinf(phi) * std::cosf(theta),
            radius * std::cosf(phi),
            radius * std::sinf(phi) * std::sinf(theta),
            1.0f);
    }

    static DirectX::XMMATRIX inverseTranspose(DirectX::CXMMATRIX M)
    {
        DirectX::XMMATRIX A = M;
        A.r[3] = DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);

        DirectX::XMVECTOR det = DirectX::XMMatrixDeterminant(A);
        return DirectX::XMMatrixTranspose(DirectX::XMMatrixInverse(&det, A));
    }

    static DirectX::XMMATRIX transposeFromXMFloat(const DirectX::XMFLOAT4X4& m)
    {
        return DirectX::XMMatrixTranspose(XMLoadFloat4x4(&m));
    }

    static DirectX::XMFLOAT4X4 identity4x4()
    {
        static DirectX::XMFLOAT4X4 I(
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f);

        return I;
    }

    static std::string printMatrix(const DirectX::XMFLOAT4X4& m, bool toLog = false);
    static DirectX::XMVECTOR randUnitVec3();
    static DirectX::XMVECTOR randHemisphereUnitVec3(DirectX::XMVECTOR n);

    static const float Infinity;
    static const float Pi;
};