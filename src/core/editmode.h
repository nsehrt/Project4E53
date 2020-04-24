#pragma once
#include "../util/d3dUtil.h"

enum class EditTool
{
    Height,
    Paint
};

struct EditSettings
{
    DirectX::XMFLOAT2 Position = DirectX::XMFLOAT2(0.0f,0.0f);
    float Velocity = 0.0f;
    float BaseVelocity = 150.0f;
    float BaseSelectSize = 60.0f;
    float BaseSelectSizeMax;

    float BaseRadius = 8.0f;
    float FallOffRadius = 4.0f;
    float FallOffRatio = 0.5f;

    float heightIncrease = 10.0f;
    float paintIncrease = 0.25f;
    int usedTextureIndex = 0;

    float resetHeight = 0.0f;

    EditTool toolMode = EditTool::Height;
    bool WireFrameOn = false;

    const float heightIncreaseMin = 1.0f;
    const float paintIncreaseMin = 0.05f;
    const float heightIncreaseMax = 100.0f;
    const float paintIncreaseMax = 1.0f;
    const float fallOffRatioMin = 0.05f;
    const float fallOffRatioMax = 0.95f;

    const int textureMax = 4;
};