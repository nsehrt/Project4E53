#include "../util/collisiondatabase.h"
#include "../util/serviceprovider.h"
#include "../extern/json.hpp"

using json = nlohmann::json;



bool CollisionDatabase::load()
{
    std::ifstream inStream(path);
    json db;

    if(!inStream.is_open())
    {
        LOG(Severity::Critical, "Unable to open collision database!");
        return false;
    }


    try
    {
        db = json::parse(inStream);
    }
    catch(nlohmann::detail::parse_error)
    {
        LOG(Severity::Critical, "Error while parsing collision database! Check JSON validity!")
            return false;
    }
    catch(...)
    {
        LOG(Severity::Critical, "Unknown error with collision database!")
            return false;
    }

    inStream.close();


    for(const auto& entry : db["collisions"])
    {
        DirectX::XMFLOAT3 tExtents = { entry["Extents"][0], entry["Extents"][1] ,entry["Extents"][2] };
        add(entry["Name"], entry["Type"], tExtents);
    }

    return true;
}

bool CollisionDatabase::save()
{
    json saveFile;

    for(const auto& e : database)
    {
        json t;
        t["Name"] = e.first;
        t["Type"] = database[e.first].shapeType;
        t["Extents"][0] = database[e.first].extents.x;
        t["Extents"][1] = database[e.first].extents.y;
        t["Extents"][2] = database[e.first].extents.z;
        saveFile["collisions"].push_back(t);
    }

    std::ofstream file(path);

    if(!file.is_open())
    {
        LOG(Severity::Error, "Can not write collision database to " << path << "!");
        return false;
    }

    file << saveFile.dump(4);
    file.close();

    return true;
}

bool CollisionDatabase::add(const std::string& modelName, int shapeType, DirectX::XMFLOAT3& extents)
{
    database[modelName] = CollisionInfo{ shapeType, extents };
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
