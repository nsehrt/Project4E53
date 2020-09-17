#include "../util/collisiondatabase.h"
#include "../extern/json.hpp"

using json = nlohmann::json;



bool CollisionDatabase::load()
{
    return true;
}

bool CollisionDatabase::save()
{
    return true;
}

bool CollisionDatabase::add(const std::string& modelName, int shapeType, DirectX::XMFLOAT3& extents)
{
    return true;
}

int CollisionDatabase::getShapeType(const std::string& modelName)
{
    return database[modelName].shapeType;
}

DirectX::XMFLOAT3& CollisionDatabase::getExtents(const std::string& modelName)
{
    return database[modelName].extents;
}
