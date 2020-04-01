#include "gameobject.h"
#include "camera.h"
#include "../util/serviceprovider.h"
#include "../extern/json.hpp"

#define LEVEL_PATH "data/level"

using json = nlohmann::json;

class Level
{
public:
    explicit Level() = default;
    ~Level() = default;

    bool load(const std::string& levelFile);

    void update(const GameTime& gt);
    void updateBuffers(const GameTime& gt);
    void draw();

    std::unordered_map<std::string, std::unique_ptr<GameObject>> mGameObjects;
    std::vector<std::unique_ptr<Camera>> mCameras;

    Camera* activeCamera = nullptr;

    /*frame resource related*/
    void cycleFrameResource();
    int getCurrentFrameResourceIndex();
    FrameResource* getCurrentFrameResource();

private:

    int amountGameObjects = 0;

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