#pragma once

#include "../util/serviceprovider.h"
#include "../extern/json.hpp"
#include "gameobject.h"

#define LEVEL_PATH "data/level"

using json = nlohmann::json;

class Level
{
public:
    explicit Level() = default;
    ~Level() = default;

    /* load a level file in data/level/ */
    bool load(const std::string& levelFile);

    /* update all game objects in the level */
    void update(const GameTime& gt);

    /* draw the level */
    void draw();

    void drawShadow();

    std::unordered_map<std::string, std::unique_ptr<GameObject>> mGameObjects;
    std::vector<std::shared_ptr<Camera>> mCameras;

private:

    /* total amount of game objects in the level, includes sky sphere*/
    int amountGameObjects = 0;

    bool parseSky(const json& skyJson);
    bool parseCameras(const json& cameraJson);
    bool parseGameObjects(const json& gameObjectJson);



    bool exists(const nlohmann::json& j, const std::string& key)
    {
        return j.find(key) != j.end();
    }

    friend class RenderResource;
};