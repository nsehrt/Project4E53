#pragma once

#include "../extern/json.hpp"
#include "../core/gameobject.h"
#include "../util/serviceprovider.h"
#include "../render/lightobject.h"

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

    std::unordered_map<std::string, std::unique_ptr<LightObject>> mLightObjects;
    XMFLOAT3 AmbientLight = { 0.25f, 0.25f, 0.25f };

    std::vector<std::shared_ptr<Camera>> mCameras;

    CD3DX12_GPU_DESCRIPTOR_HANDLE defaultCubeMapHandle;

private:

    /* total amount of game objects in the level, includes sky sphere*/
    int amountGameObjects = 0;

    bool parseSky(const json& skyJson);
    bool parseLights(const json& lightJson);
    bool parseCameras(const json& cameraJson);
    bool parseGameObjects(const json& gameObjectJson);

    /*render order*/
    std::vector<std::vector<GameObject*>> renderOrder;
    std::vector<std::vector<GameObject*>> shadowRenderOrder;

    bool exists(const nlohmann::json& j, const std::string& key)
    {
        return j.find(key) != j.end();
    }

};