#pragma once

class GameObject;

#include "../util/d3dUtil.h"
#include <iostream>

class QuadTree
{

public:

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

    /*
    @returns raw pointer to the root node of the tree
    */
    QuadNode* getRoot() const
    {
        return root.get();
    }

    /*
    seach the tree for collisions with game object obj
    @param object for which collisions are searched for
    @param pointer to game objects that collide with obj will be stored here
    */
    void searchCollision(GameObject* obj, std::vector<GameObject*>& collisions) const;

    /*
    compute collision between all nodes in the tree and a camera frustum
    @param frustum that is used for collision check
    @param pointer to nodes that collide with frustum will be stored here
    */
    void searchCollision(const DirectX::BoundingFrustum& frustum, std::vector<QuadNode*>& nodes) const;

    /*outputs the quadtree to a stream*/
    friend std::ostream& operator<<(std::ostream& os, const QuadTree& tree);

    /*
    @returns string that describes the quad tree
    */
    std::string toString() const;

private:
    const float fixedHeight = 64.0f;
    int totalPointersStored = 0;

    std::unique_ptr<QuadNode> populateNode(DirectX::XMFLOAT3 center, float x, float z, int depth);
    std::unique_ptr<QuadNode> root = nullptr;

};