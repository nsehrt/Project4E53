#pragma once
#include "../util/d3dUtil.h"
#include "../core/gameobject.h"

enum class EditTool
{
    Height,
    Paint,
    Object
};

enum class ObjectTransformTool : int
{
    Translation,
    Scale,
    Rotation
};

enum class TranslationAxis : int
{
    XY,
    XZ,
    X,
    Y, 
    Z
};

enum class ScaleAxis : int
{
    XYZ,
    X,
    Y,
    Z
};

enum class RotationAxis : int
{
    X,
    Y,
    Z
};

struct EditSettings
{
    EditTool toolMode = EditTool::Height;
    bool WireFrameOn = false;

    /*selection*/
    DirectX::XMFLOAT2 Position = DirectX::XMFLOAT2(0.0f,0.0f);
    float Velocity = 0.0f;
    float BaseVelocity = 150.0f;
    float BaseSelectSize = 60.0f;
    float BaseSelectSizeMax;

    float BaseRadius = 8.0f;
    float FallOffRadius = 4.0f;
    float FallOffRatio = 0.5f;


    /*height*/

    float heightIncrease = 10.0f;

    float resetHeight = 0.0f;

    const float heightIncreaseMin = 1.0f;
    const float heightIncreaseMax = 100.0f;


    /*paint*/
    float paintIncrease = 0.25f;
    int usedTextureIndex = 0;

    const float paintIncreaseMin = 0.05f;
    const float paintIncreaseMax = 1.0f;
    const float fallOffRatioMin = 0.05f;
    const float fallOffRatioMax = 0.95f;

    const int textureMax = 4;

    /*game object tool*/
    GameObject* currentSelection = nullptr;
    int currentSelectionIndex = -1;

    ObjectTransformTool objTransformTool = ObjectTransformTool::Translation;
    /*per tool axis control*/
    TranslationAxis translationAxis = TranslationAxis::XY;
    ScaleAxis scaleAxis = ScaleAxis::XYZ;
    RotationAxis rotationAxis = RotationAxis::X;

    const float translationIncreaseBase = 10.0f;
    const float scaleIncreaseBase = 2.0f;
    const float rotationIncreaseBase = XM_PIDIV2;

    /*gui*/
    float savedAnim = 0.0f;
    bool saveSuccess = true;

    float legendAnim = 0.0f;
    bool legendStatus = false;
    const float legenAnimDur = 0.25f;
};