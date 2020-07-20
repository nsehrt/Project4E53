#include "../core/animationevaluator.h"

void AnimationEvaluator::interpolate(const AnimationClip* clip, const float timePos, std::vector<DirectX::XMFLOAT4X4>& localTransforms)
{

    for (UINT i = 0; i < localTransforms.size(); ++i)
    {
        if (!clip->boneAnimations[i].isEmpty)
            clip->boneAnimations[i].interpolate(timePos, localTransforms[i]);
    }

}
