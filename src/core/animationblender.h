#pragma once

#include "../util/mathhelper.h"
#include "../render/renderstructs.h"

class AnimationBlender
{

public:

    struct AnimationBlendData
    {
        AnimationClip* animation = nullptr;
        std::vector<std::pair<float, float>> blendPairs;
    };

    explicit AnimationBlender() = default;
    ~AnimationBlender() = default;


    void setBlendData(std::vector<AnimationBlender::AnimationBlendData>& _data);
    void blend(const float deltaTime, const float animationPercent, std::vector<DirectX::XMFLOAT4X4>& localTransforms);

private:
    void keyFrameLerp(const KeyFrame& a, const KeyFrame& b, float lerpPercent, DirectX::XMFLOAT4X4& matrix);

    std::vector<AnimationBlendData> data;

    int clipA = -1;
    int clipB = -1;

    std::vector<KeyFrame> returnedDataClipA;
    std::vector<KeyFrame> returnedDataClipB;
    float currentPrimaryEndTime = 0.0f;
    float animationTimer = 0.0f;
    bool dataInitialised = false;
};