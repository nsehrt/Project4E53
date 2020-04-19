#pragma once
#include "../util/d3dUtil.h"

struct EditSelect
{
    DirectX::XMFLOAT2 Position;
    float Velocity = 0.0f;
    float FallOffStart = 5.0f;
    float FallOffEnd = 7.5f;
    float BaseVelocity = 150.0f;
    float BaseSelectSize = 20.0f;
};