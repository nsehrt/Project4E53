#include "quadtree.h"

using namespace DirectX;


void QuadTree::build(DirectX::XMFLOAT3 center, float x, float z, int depth)
{
    if (depth < 1) depth = 1;

    root = populateNode(center, x, z, depth);
}

std::unique_ptr<QuadTree::QuadNode> QuadTree::populateNode(DirectX::XMFLOAT3 center, float x, float z, int depth)
{
    auto node = std::make_unique<QuadNode>();
    auto box = DirectX::BoundingBox(center, { x, fixedHeight, z });

    node->boundingBox = box;

    if (depth > 0)
    {
        for (auto i = 0; i < 4; i++)
        {

            
        }
    }


    return node;
}


