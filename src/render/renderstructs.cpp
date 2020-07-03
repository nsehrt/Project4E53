#include "renderstructs.h"

using namespace DirectX;

void BoneAnimation::interpolate(float time, XMFLOAT4X4& matrix) const
{
    if (time <= keyFrames.front().timeStamp)
    {
        XMVECTOR S = XMLoadFloat3(&keyFrames.front().scale);
        XMVECTOR P = XMLoadFloat3(&keyFrames.front().translation);
        XMVECTOR Q = XMLoadFloat4(&keyFrames.front().rotationQuat);

        XMVECTOR zero = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
        XMStoreFloat4x4(&matrix, XMMatrixAffineTransformation(S, zero, Q, P));
    }
    else if (time >= keyFrames.back().timeStamp)
    {
        XMVECTOR S = XMLoadFloat3(&keyFrames.back().scale);
        XMVECTOR P = XMLoadFloat3(&keyFrames.back().translation);
        XMVECTOR Q = XMLoadFloat4(&keyFrames.back().rotationQuat);

        XMVECTOR zero = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
        XMStoreFloat4x4(&matrix, XMMatrixAffineTransformation(S, zero, Q, P));
    }
    else
    {
        for (UINT i = 0; i < keyFrames.size() - 1; ++i)
        {
            if (time >= keyFrames[i].timeStamp && time <= keyFrames[(INT_PTR)i + 1].timeStamp)
            {
                float lerpPercent = (time - keyFrames[i].timeStamp) / (keyFrames[(INT_PTR)i + 1].timeStamp - keyFrames[i].timeStamp);

                XMVECTOR s0 = XMLoadFloat3(&keyFrames[i].scale);
                XMVECTOR s1 = XMLoadFloat3(&keyFrames[(INT_PTR)i + 1].scale);

                XMVECTOR p0 = XMLoadFloat3(&keyFrames[i].translation);
                XMVECTOR p1 = XMLoadFloat3(&keyFrames[(INT_PTR)i + 1].translation);

                XMVECTOR q0 = XMLoadFloat4(&keyFrames[i].rotationQuat);
                XMVECTOR q1 = XMLoadFloat4(&keyFrames[(INT_PTR)i + 1].rotationQuat);

                XMVECTOR S = XMVectorLerp(s0, s1, lerpPercent);
                XMVECTOR P = XMVectorLerp(p0, p1, lerpPercent);
                XMVECTOR Q = XMQuaternionSlerp(q0, q1, lerpPercent);

                XMVECTOR zero = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
                XMStoreFloat4x4(&matrix, XMMatrixAffineTransformation(S, zero, Q, P));

                break;
            }
        }
    }
}



float AnimationClip::getStartTime()
{
    if (startTime != -1.0f)
    {
        return startTime;
    }

    float result = MathHelper::Infinity;

    for (const auto& i : boneAnimations)
    {
        result = MathHelper::minH(result, i.getStartTime());
    }

    startTime = result;

    return result;
}



float AnimationClip::getEndTime()
{
    if (endTime != -1.0f)
    {
        return endTime;
    }

    float result = 0.0f;

    for (const auto& i : boneAnimations)
    {
        result = MathHelper::maxH(result, i.getEndTime());
    }

    endTime = result;

    return result;
}

void AnimationClip::interpolate(float t, std::vector<XMFLOAT4X4>& boneTransforms) const
{
    for (UINT i = 0; i < boneTransforms.size(); ++i)
    {
        boneAnimations[i].interpolate(t, boneTransforms[i]);
    }
}

void SkinnedModel::calculateFinalTransforms(AnimationClip* currentClip, float timePos)
{
    UINT numBones = (UINT)boneOffsets.size();

    std::vector<XMFLOAT4X4> toParentTransforms(numBones);

    currentClip->interpolate(timePos, toParentTransforms);

    std::vector<XMFLOAT4X4> toRootTransforms(numBones);

    toRootTransforms[0] = toParentTransforms[0];

    // find the toRootTransform of the children.
    for (UINT i = 1; i < numBones; ++i)
    {
        XMMATRIX toParent = XMLoadFloat4x4(&toParentTransforms[i]);

        int parentIndex = boneHierarchy[i];
        XMMATRIX parentToRoot = XMLoadFloat4x4(&toRootTransforms[parentIndex]);

        XMMATRIX toRoot = XMMatrixMultiply(toParent, parentToRoot);

        XMStoreFloat4x4(&toRootTransforms[i], toRoot);
    }

    // Premultiply by the bone offset transform to get the final transform.
    for (UINT i = 0; i < numBones; ++i)
    {
        XMMATRIX offset = XMLoadFloat4x4(&boneOffsets[i]);
        XMMATRIX toRoot = XMLoadFloat4x4(&toRootTransforms[i]);

        /*multiply the global transform inverse*/
        XMMATRIX globalInverse = XMMatrixRotationQuaternion(XMVectorSet(0, 1, 0, 0));
        auto det = XMMatrixDeterminant(globalInverse);
        XMMATRIX finalTransform = XMMatrixMultiply(XMMatrixInverse(&det, globalInverse), XMMatrixMultiply(offset, toRoot)); //


        //XMMATRIX finalTransform = XMMatrixMultiply(offset, toRoot);
        XMStoreFloat4x4(&finalTransforms[i], XMMatrixTranspose(finalTransform));
    }
}