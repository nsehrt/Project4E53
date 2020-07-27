#pragma once

class GameObject;

#include "../util/d3dUtil.h"

class QuadTree
{
private:
    struct QuadNode
    {
        DirectX::BoundingBox boundingBox;
        std::vector<GameObject*> containedObjects;
        std::vector<std::unique_ptr<QuadNode>> children;
    };

public:
    explicit QuadTree() = default;
    ~QuadTree() = default;

    void build(DirectX::XMFLOAT3 center, float x, float z, int depth);

private:
    const float fixedHeight = 2048.0f;

    std::unique_ptr<QuadNode> populateNode(DirectX::XMFLOAT3 center, float x, float z, int depth);

    std::unique_ptr<QuadNode> root = nullptr;
};