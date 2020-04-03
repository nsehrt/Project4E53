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

    /* update constant buffers for the gpu */
    void updateBuffers(const GameTime& gt);

    /* draw the level */
    void draw();

    std::unordered_map<std::string, std::unique_ptr<GameObject>> mGameObjects;
    std::vector<std::shared_ptr<Camera>> mCameras;

    //Camera* activeCamera = nullptr;

    /*frame resource related*/

    /*use next frame resource (start of new frame) */
    void cycleFrameResource();

    /*return the index of the current frame resource */
    int getCurrentFrameResourceIndex();

    /* return pointer to the current frame resource */
    FrameResource* getCurrentFrameResource();

private:

    /* total amount of game objects in the level, includes sky sphere*/
    int amountGameObjects = 0;

    bool parseSky(const json& skyJson);
    bool parseCameras(const json& cameraJson);
    bool parseGameObjects(const json& gameObjectJson);
    
    /*frame resource related*/
    std::vector<std::unique_ptr<FrameResource>> mFrameResources;
    FrameResource* mCurrentFrameResource = nullptr;
    int mCurrentFrameResourceIndex = 0;

    PassConstants mMainPassConstants;

    void buildFrameResource();

    void updateGameObjectConstantBuffers(const GameTime& gt);
    void updateMainPassConstantBuffers(const GameTime& gt);
    void updateMaterialConstantBuffers(const GameTime& gt);




    bool exists(const nlohmann::json& j, const std::string& key)
    {
        return j.find(key) != j.end();
    }

    friend class RenderResource;
};