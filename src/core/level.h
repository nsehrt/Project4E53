#pragma once

#include "../extern/json.hpp"
#include "../core/gameobject.h"
#include "../util/serviceprovider.h"
#include "../core/lightobject.h"
#include "../core/terrain.h"

#define LEVEL_PATH "data/level"

#define MAX_LIGHTS 8
#define AMOUNT_DIRECTIONAL 3
#define AMOUNT_SPOT 1

using json = nlohmann::json;

class Level
{
public:
    explicit Level()
    {
        for (int i = 0; i < mCurrentLightObjects.size(); i++)
            mCurrentLightObjects[i] = nullptr;
    };
    ~Level() = default;

    /* load a level file in data/level/ */
    bool load(const std::string& levelFile);

    /* update all game objects in the level */
    void update(const GameTime& gt);
    void updateWater(const GameTime& gt);

    /* draw the level */
    void drawWater();
    void drawTerrain();
    void draw();

    /*save gamobjects terrain etc*/
    bool save();

    /*create new level*/
    bool createNew(const std::string& levelFile);
    static bool levelExists(const std::string& levelFile);

    void drawShadow();

    void calculateRenderOrder();

    void addGameObject(json goJson)
    {
        mGameObjects[goJson["Name"]] = std::make_unique<GameObject>(goJson, amountObjectCBs);
        amountObjectCBs += 4;
        calculateRenderOrder();
    }

    std::unique_ptr<Terrain> mTerrain;

    std::unordered_map<std::string, std::unique_ptr<GameObject>> mGameObjects;

    std::array<LightObject*, MAX_LIGHTS> mCurrentLightObjects;
    std::vector<std::unique_ptr<LightObject>> mLightObjects;

    XMFLOAT4 AmbientLight = { 0.25f, 0.25f, 0.25f, 1.0f };

    std::vector<std::shared_ptr<Camera>> mCameras;

    CD3DX12_GPU_DESCRIPTOR_HANDLE defaultCubeMapHandle;
    std::string defaultCubeMapStr;
    std::string skyMaterial;
    std::string loadedLevel;

private:

    /* total amount of game objects in the level, includes sky sphere*/
    int amountObjectCBs = 0;
    int amountGameObjects = 0;

    bool parseSky(const json& skyJson);
    bool parseLights(const json& lightJson);
    bool parseCameras(const json& cameraJson);
    bool parseGameObjects(const json& gameObjectJson);
    bool parseTerrain(const json& terrainJson);
    bool parseWater(const json& waterJson);

    /*render order*/
    std::vector<std::vector<GameObject*>> renderOrder;
    std::vector<std::vector<GameObject*>> shadowRenderOrder;

    bool exists(const nlohmann::json& j, const std::string& key)
    {
        return j.find(key) != j.end();
    }
};