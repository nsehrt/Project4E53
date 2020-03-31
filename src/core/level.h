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
    void draw();

    std::unordered_map<std::string, std::unique_ptr<GameObject>> mGameObjects;
    std::vector<std::unique_ptr<Camera>> mCameras;

private:

    int amountGameObjects = 0;

    bool parseCameras(const json& cameraJson);
    bool parseGameObjects(const json& gameObjectJson);
    


    bool exists(const nlohmann::json& j, const std::string& key)
    {
        return j.find(key) != j.end();
    }

};