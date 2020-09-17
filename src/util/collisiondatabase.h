#pragma once

#include <unordered_map>
#include "../util/mathhelper.h"

struct CollisionInfo
{
    int shapeType = 0;
    DirectX::XMFLOAT3 extents;
};

class CollisionDatabase
{
public:
    CollisionDatabase() = default;
    ~CollisionDatabase() = default;

    bool load();
    bool save();
    bool add(const std::string& modelName, int shapeType, DirectX::XMFLOAT3& extents);
    
    int getShapeType(const std::string& modelName);
    DirectX::XMFLOAT3& getExtents(const std::string& modelName);

private:
    const std::string path = "/data/cdb/data.json";
    std::unordered_map<std::string, CollisionInfo> database;
};