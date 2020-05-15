#pragma once

#include "../extern/json.hpp"
#include "../core/gameobject.h"
#include "../util/serviceprovider.h"
#include "../core/lightobject.h"
#include "../core/terrain.h"
#include "../core/water.h"
#include "../core/grass.h"

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

    /*needed because a different root signature is used*/
    void drawTerrain();

    /* draw the level */
    void draw();

    /*save gamobjects terrain etc*/
    bool save();

    /*create new level*/
    bool createNew(const std::string& levelFile);

    /*checks if level file exists*/
    static bool levelExists(const std::string& levelFile);

    /*shadow equivalent of draw()*/
    void drawShadow();

    /*need to call this if new object is added or render type is changed*/
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
    std::vector<std::unique_ptr<Water>> mWater;
    std::vector<std::unique_ptr<Grass>> mGrass;

    XMFLOAT4 AmbientLight = { 0.25f, 0.25f, 0.25f, 1.0f };

    std::vector<std::shared_ptr<Camera>> mCameras;

    CD3DX12_GPU_DESCRIPTOR_HANDLE defaultCubeMapHandle;
    std::string defaultCubeMapStr;
    std::string skyMaterial;
    std::string loadedLevel;

private:

    float waterUpdate = 0.0f;

    /* total amount of game objects in the level, includes sky sphere*/
    int amountObjectCBs = 0;
    int amountGameObjects = 0;

    bool parseSky(const json& skyJson);
    bool parseLights(const json& lightJson);
    bool parseCameras(const json& cameraJson);
    bool parseGameObjects(const json& gameObjectJson);
    bool parseTerrain(const json& terrainJson);
    bool parseGrass(const json& grassJson);
    bool parseWater(const json& waterJson);

    /*render order*/
    std::vector<std::vector<GameObject*>> renderOrder;
    std::vector<std::vector<GameObject*>> shadowRenderOrder;

    bool exists(const nlohmann::json& j, const std::string& key)
    {
        return j.find(key) != j.end();
    }
};