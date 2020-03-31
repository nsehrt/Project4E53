#include "level.h"

bool Level::load(const std::string& levelFile)
{
    /*open and parse the level file*/
    std::ifstream levelStream (LEVEL_PATH + std::string("/") + levelFile);

    if (!levelStream.is_open())
    {
        LOG(Severity::Critical, "Unable to open the level file " << levelFile << "!");
        return false;
    }

    json levelJson;

    try
    {
        levelJson = json::parse(levelStream);
    }
    catch(...){
        LOG(Severity::Critical, "Error while parsing level file! Check JSON validity!")
        return false;
    }

    levelStream.close();

    LOG(Severity::Info, "Parsing of level file " << levelFile << " successful");

    /*parse cameras*/
    if (!parseCameras(levelJson["Camera"]))
    {

    }

    /*parse terrain*/



    /*parse game objects*/
    if (!parseGameObjects(levelJson["GameObject"]))
    {

    }


    return true;
}

bool Level::parseCameras(const json& cameraJson)
{
    return true;
}

bool Level::parseGameObjects(const json& gameObjectJson)
{
    for (auto const& entryJson : gameObjectJson)
    {

        if (!exists(entryJson, "Name") ||
            !exists(entryJson, "Model") ||
            !exists(entryJson, "Material") ||
            !exists(entryJson, "Position") ||
            !exists(entryJson, "Rotation") ||
            !exists(entryJson, "Scale") ||
            !exists(entryJson, "CollisionEnabled") ||
            !exists(entryJson, "DrawEnabled") ||
            !exists(entryJson, "ShadowEnabled") )
        {
            LOG(Severity::Warning, "Skipping game object due to missing properties!");
            continue;
        }

        auto gameObject = std::make_unique<GameObject>(entryJson, amountGameObjects++);

        mGameObjects[gameObject->name] = std::move(gameObject);

    }


    return true;
}
