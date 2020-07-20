#pragma once

#include <DirectXMath.h>
#include "../render/renderstructs.h"

class AnimationEvaluator
{

public:

    explicit AnimationEvaluator() = default;
    ~AnimationEvaluator() = default;

    void interpolate(const AnimationClip* clip, const float timePos, std::vector<DirectX::XMFLOAT4X4>& localTransforms);

private:

};