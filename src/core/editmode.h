#pragma once

#include "../util/d3dUtil.h"
#include "../core/gameobject.h"

enum class EditTool
{
    Height,
    Paint,
    ObjectTransform,
    ObjectMeta,
    Light,
    Camera
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

enum class LightColorAxis : int
{
    R,
    G,
    B
};

enum class LightDirectionAxis : int
{
    X,
    Y,
    Z
};

enum class LightTranslationAxis : int
{
    XZ,
    XY
};

enum class LightTypeChoice : int
{
    Directional,
    Point
};

enum class GameObjectProperty : int
{
    Collision,
    Draw,
    Shadow,
    ShadowForce
};

struct EditSettings
{
    EditTool toolMode = EditTool::Height;
    EditTool prevTool = EditTool::Height;

    bool WireFrameOn = false;

    /*selection*/
    DirectX::XMFLOAT2 Position = DirectX::XMFLOAT2(0.0f, 0.0f);
    float Velocity = 0.0f;
    float BaseVelocity = 150.0f;
    float BaseSelectSize = 20.0f;

    float BaseRadius = 8.0f;
    float FallOffRadius = 4.0f;
    float FallOffRatio = 0.5f;

    /*height*/

    float heightIncrease = 10.0f;

    float resetHeight = 0.0f;

    const float heightIncreaseMin = 1.0f;
    const float heightIncreaseMax = 100.0f;

    /*paint*/
    float paintIncrease = 0.5f;
    int usedTextureIndex = 0;

    const float paintIncreaseMin = 0.05f;
    const float paintIncreaseMax = 1.0f;
    const float fallOffRatioMin = 0.05f;
    const float fallOffRatioMax = 0.95f;

    const int textureMax = 4;

    /*light*/
    LightTypeChoice lightTypeChoice = LightTypeChoice::Point;
    LightColorAxis lightColorAxis = LightColorAxis::R;
    LightTranslationAxis lightTranslationAxis = LightTranslationAxis::XZ;
    LightDirectionAxis lightDirectionAxis = LightDirectionAxis::X;

    UINT currentLightSelectionIndex = 3;
    UINT currentLightSelectionDirectionIndex = 0;
    UINT currentLightSelectionPointIndex = 3;

    /*model switch*/
    std::map<std::string, std::vector<Model*>> orderedModels;
    std::vector<Model*>::iterator selectedModel;
    std::string selectedGroup = "";


    /*game object tool*/
    GameObject* currentSelection = nullptr;
    int currentSelectionIndex = -1;

    ObjectTransformTool objTransformTool = ObjectTransformTool::Translation;
    /*per tool axis control*/
    TranslationAxis translationAxis = TranslationAxis::XY;
    ScaleAxis scaleAxis = ScaleAxis::XYZ;
    RotationAxis rotationAxis = RotationAxis::X;

    const float translationIncreaseBase = 60.0f;
    const float scaleIncreaseBase = 0.5f;
    const float rotationIncreaseBase = XM_PIDIV2;

    /*game object meta tool*/
    GameObjectProperty gameObjectProperty = GameObjectProperty::Collision;

    /*gui*/
    float savedAnim = 0.0f;
    bool saveSuccess = true;

    float legendAnim = 0.0f;
    bool legendStatus = false;
    const float legenAnimDur = 0.25f;
};