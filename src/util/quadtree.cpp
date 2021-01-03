#include "quadtree.h"
#include "../core/gameobject.h"
#include "../util/serviceprovider.h"

using namespace DirectX;


void QuadTree::build(DirectX::XMFLOAT3 center, float x, float z, int depth)
{
    if (depth < 1) depth = 1;

    root = populateNode(center, x, z, depth);
}

bool QuadTree::insert(GameObject* gO)
{
    bool inserted = false;
    auto box = gO->getCollider().getFrustumBox();

    const std::function<void(QuadNode*)> recursiveInsert = [&](QuadNode* node)
    {
        if (node->boundingBox.Intersects(box))
        {
            if (node->children.empty())
            {
                totalPointersStored++;
                node->containedObjects.push_back(gO);
                inserted = true;
            }
            else
            {
                for (auto& c : node->children)
                {
                    recursiveInsert(c.get());
                }
            }
        }
    };

    recursiveInsert(root.get());

    return inserted;
}

std::unique_ptr<QuadTree::QuadNode> QuadTree::populateNode(DirectX::XMFLOAT3 center, float x, float z, int depth)
{
    auto node = std::make_unique<QuadNode>();
    auto box = DirectX::BoundingBox(center, { x / 2.0f, fixedHeight, z / 2.0f});

    node->boundingBox = box;

    if (depth > 0)
    {
        auto center1 = XMFLOAT3{ center.x - x / 4.0f, 0.0f, center.z - z / 4.0f };
        auto x1 = populateNode(center1, x / 2.0f, z / 2.0f, depth - 1);
        node->children.push_back(std::move(x1));

        auto center2 = XMFLOAT3{center.x + x / 4.0f, 0.0f, center.z - z / 4.0f };
        auto x2 = populateNode(center2, x / 2.0f, z / 2.0f, depth - 1);
        node->children.push_back(std::move(x2));

        auto center3 = XMFLOAT3{center.x - x / 4.0f, 0.0f, center.z + z / 4.0f };
        auto x3 = populateNode(center3, x / 2.0f, z / 2.0f, depth - 1);
        node->children.push_back(std::move(x3));

        auto center4 = XMFLOAT3{center.x + x / 4.0f, 0.0f, center.z + z / 4.0f };
        auto x4 = populateNode(center4, x / 2.0f, z / 2.0f, depth - 1);
        node->children.push_back(std::move(x4));
    }


    return node;
}


//void QuadTree::searchCollision(GameObject* obj, std::vector<GameObject*>& collisions) const
//{
//    std::vector<QuadNode*> nodeCollisions;
//    UINT checks = 0;
//
//    /*skip if obj has no collision*/
//    if (!obj->isCollisionEnabled) return;
//
//    /*look up which quadtree nodes collide with the object bounding box*/
//    const std::function<void(QuadNode*)> recursiveSearch = [&](QuadNode* node)
//    {
//        if (node->children.empty())
//        {
//            checks++;
//            if (node->boundingBox.Intersects(obj->getCollider().getFrustumBox()))
//            {
//                nodeCollisions.push_back(node);
//            }
//        }
//        else
//        {
//            checks++;
//            if (node->boundingBox.Intersects(obj->getCollider().getFrustumBox()))
//            {
//                for (auto& i : node->children)
//                {
//                    recursiveSearch(i.get());
//                }
//            }
//        }
//    };
//
//    recursiveSearch(getRoot());
//
//    /*call intersect on every game object in every collided node*/
//    for (const auto& i : nodeCollisions)
//    {
//        for (const auto& go : i->containedObjects)
//        {
//            if (!go->isCollisionEnabled) continue;
//
//            checks++;
//            if (go->getCollider().intersects(obj->getCollider()))
//            {
//                collisions.push_back(go);
//            }
//        }
//    }
//
//    //LOG(Severity::Debug, "Checks: " << checks << " in " << nodeCollisions.size() << " nodes.");
//}

void QuadTree::searchCollision(const DirectX::BoundingFrustum& frustum, std::vector<QuadNode*>& nodes) const
{

    UINT checks = 0;

    /*look up which quadtree nodes collide with the frustum*/
    const std::function<void(QuadNode*)> recursiveSearch = [&](QuadNode* node)
    {
        if (node->children.empty())
        {
            checks++;
            if (frustum.Contains(node->boundingBox) != DirectX::DISJOINT && !node->containedObjects.empty())
            {
                nodes.push_back(node);
            }
        }
        else
        {
            checks++;
            if (frustum.Contains(node->boundingBox) != DirectX::DISJOINT)
            {
                for (auto& i : node->children)
                {
                    recursiveSearch(i.get());
                }
            }
        }
    };

    recursiveSearch(getRoot());

    //LOG(Severity::Debug, "Checks: " << checks << " and " << nodes.size() << " collision nodes.");
}


std::string QuadTree::toString() const
{
    std::stringstream result;

    std::function<void(QuadNode*, int)> addNodeToString = [&](QuadNode* node, int depth) -> void
    {
        result << std::string((long long)depth * 3, ' ') << *node << "\n";

        for (const auto& i : node->children)
        {
            addNodeToString(i.get(), depth + 1);
        }
    };

    addNodeToString(root.get(), 0);

    return result.str();
}


std::ostream& operator<<(std::ostream& os, const QuadTree& tree)
{
    os << tree.toString() << "\n";
    return os;
}
