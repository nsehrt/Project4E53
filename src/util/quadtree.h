#pragma once

class GameObject;

#include "../util/d3dUtil.h"
#include <iostream>

class QuadTree
{
private:
    struct QuadNode
    {
        DirectX::BoundingBox boundingBox;
        std::vector<GameObject*> containedObjects;
        std::vector<std::unique_ptr<QuadNode>> children;

        friend std::ostream& operator<< (std::ostream& os, const QuadNode& node)
        {
            os << node.boundingBox.Center.x << " | " << node.boundingBox.Center.z << " (" << node.boundingBox.Extents.x << " | " << node.boundingBox.Extents.z << ") -> " << node.containedObjects.size() << " contained objects.";
            return os;
        }
    };

public:
    /*default constructor*/
    explicit QuadTree() = default;

    /*default destructor*/
    ~QuadTree() = default;

    /*builds the empty quadtree skeleton
    @param center of the area
    @param width
    @param length
    @param how deep the quad tree will subdivide the area
    */
    void build(DirectX::XMFLOAT3 center, float x, float z, int depth);

    /*
    inserts a game object at the right position(s) in the tree
    @params the game object you want to insert
    */
    bool insert(GameObject* gO);

    /*
    @returns how many game object pointers are stored in the tree (duplicates possible)
    */
    int sizeContainedObjects() const
    {
        return totalPointersStored;
    }

    /*outputs the quadtree to a stream*/
    friend std::ostream& operator<<(std::ostream& os, const QuadTree& tree);

    /*
    @returns string that describes the quad tree
    */
    std::string toString() const;

private:
    const float fixedHeight = 2048.0f;
    int totalPointersStored = 0;

    std::unique_ptr<QuadNode> populateNode(DirectX::XMFLOAT3 center, float x, float z, int depth);
    std::unique_ptr<QuadNode> root = nullptr;

};