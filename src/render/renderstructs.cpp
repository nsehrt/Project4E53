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
    //if (startTime != -1.0f)
    //{
    //    return startTime;
    //}

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
    //if (endTime != -1.0f)
    //{
    //    return endTime;
    //}

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
        if(!boneAnimations[i].isEmpty)
            boneAnimations[i].interpolate(t, boneTransforms[i]);
    }
}

void SkinnedModel::calculateFinalTransforms(AnimationClip* currentClip, float timePos)
{

    std::vector<XMFLOAT4X4> localTransforms(boneCount);
    std::vector<XMFLOAT4X4> globalTransforms(boneCount);

    /*fill local transforms with the local node transforms of the bones*/
    for (auto i = 0; i < localTransforms.size(); i++)
    {
        localTransforms[i] = nodeTree.findNodeByBoneIndex(i)->transform;
    }


    currentClip->interpolate(timePos, localTransforms);


    /*calculate global transforms*/

    /*root is equal to local*/
    globalTransforms[0] = localTransforms[0];

    // find the toRootTransform of the children.
    for (UINT i = 1; i < boneCount; ++i)
    {
        XMMATRIX local = XMLoadFloat4x4(&localTransforms[i]);

        int parentIndex = boneHierarchy[i];
        XMMATRIX parentToRoot = XMLoadFloat4x4(&globalTransforms[parentIndex]);

        XMMATRIX global = XMMatrixMultiply(local, parentToRoot);
        XMStoreFloat4x4(&globalTransforms[i], global);
    }

    // Premultiply by the bone offset transform to get the final transform.
    for (UINT i = 0; i < boneCount; ++i)
    {
        XMMATRIX offset = XMLoadFloat4x4(&nodeTree.findNodeByBoneIndex(i)->transform);
        XMMATRIX toRoot = XMLoadFloat4x4(&globalTransforms[i]);
        XMMATRIX gInverse = XMLoadFloat4x4(&globalInverse);

        XMMATRIX finalTransform = gInverse * toRoot * offset;

        XMStoreFloat4x4(&finalTransforms[i], XMMatrixTranspose(finalTransform));
    }
}


Node* NodeTree::findNodeByBoneIndex(int index) const
{
    Node* retNode = nullptr;

    std::function<void(Node*)> searchTree = [&](Node* node)
    {
        if (node->boneIndex == index)
        {
            retNode = node;
        }

        for (auto i = 0; i < node->children.size(); i++)
        {
            searchTree(node->children[i]);
        }

        return;
    };

    searchTree(root);

    return retNode;
}


std::string NodeTree::toString()
{
    std::stringstream ret;
    printNodes(ret, root, 0);

    return ret.str();
}

void NodeTree::printNodes(std::stringstream& str, Node* node, int depth)
{   

    str << std::string((long long)depth * 3, ' ') << std::string(2, '-') << ">" << node->name;

    if (node->isBone)
    {
        str << " (Bone [" << node->boneIndex << "])";
    }
    else
    {
        str << " (Not a bone)";
    }

    if (node->parent != nullptr)
    {
        str << " child of node " << node->parent->name;
    }
    else
    {
        str << " root node";
    }

    DirectX::XMMATRIX trf = DirectX::XMLoadFloat4x4(&node->transform);

    if (DirectX::XMMatrixIsIdentity(trf))
    {
        str << " (Identity Transform)";
    }
    str << "\n";

    if (!DirectX::XMMatrixIsIdentity(trf))
    {
        str << MathHelper::printMatrix(node->transform);
    }
    else
    {
        str << "\n";
    }

    if (node->isBone)
    {
        str << "\nBone Offset:\n";
        str << MathHelper::printMatrix(node->boneOffset) << "\n";
    }

    for (UINT i = 0; i < node->children.size(); i++)
    {
        printNodes(str, node->children[i], depth + 1);
    }

}
