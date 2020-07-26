#include "../core/animationblender.h"
#include "../util/mathhelper.h"

using namespace DirectX;

void AnimationBlender::blend(const float deltaTime, const float animationPercent, std::vector<DirectX::XMFLOAT4X4>& localTransforms)
{
    if (!dataInitialised) return;

    int dominantAnimation = -1;

    for (const auto& i : data)
    {


    }

    animationTimer += deltaTime;

    data[clipA].animation->interpolate(animationTimer, returnedDataClipA);
    data[clipB].animation->interpolate(animationTimer, returnedDataClipB);



}

void AnimationBlender::keyFrameLerp(const KeyFrame& a, const KeyFrame& b, float lerpPercent, DirectX::XMFLOAT4X4& matrix)
{

    XMVECTOR s0 = XMLoadFloat3(&a.scale);
    XMVECTOR s1 = XMLoadFloat3(&b.scale);

    XMVECTOR p0 = XMLoadFloat3(&a.translation);
    XMVECTOR p1 = XMLoadFloat3(&b.translation);

    XMVECTOR q0 = XMLoadFloat4(&a.rotationQuat);
    XMVECTOR q1 = XMLoadFloat4(&b.rotationQuat);

    XMVECTOR S = XMVectorLerp(s0, s1, lerpPercent);
    XMVECTOR P = XMVectorLerp(p0, p1, lerpPercent);
    XMVECTOR Q = XMQuaternionSlerp(q0, q1, lerpPercent);

    XMVECTOR zero = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
    XMStoreFloat4x4(&matrix, XMMatrixAffineTransformation(S, zero, Q, P));

}

void AnimationBlender::setBlendData(std::vector<AnimationBlender::AnimationBlendData>& _data)
{
    data = _data;
    dataInitialised = true;

    currentPrimaryEndTime = data[0].animation->getEndTime();

    returnedDataClipA.resize(data.size());
    returnedDataClipB.resize(data.size());

}
