#pragma once

/*forward declaration*/
class InputManager;
class SoundEngine;
class RenderResource;
class Level;
class Camera;
class Player;
class BulletPhysics;
class CollisionDatabase;

struct EditSettings;
struct DebugInfo;

#include "../util/log.h"
#include "../util/settings.h"
#include "../core/gamestate.h"
#include <unordered_map>
#include "../input/inputmanager.h"

#define SP_ANIM(x) (ServiceProvider::getRenderResource()->mAnimations[x].get())

class ServiceProvider
{
private:

    static std::shared_ptr<Logger<LogPolicy>> vsLogger;

    static std::shared_ptr<Settings> settings;
    static std::shared_ptr<SoundEngine> audio;
    static std::shared_ptr<InputManager> input;
    static std::shared_ptr<RenderResource> renderResource;
    static BulletPhysics* physics;
    static CollisionDatabase* collisionDatabase;

    static std::shared_ptr<Player> activePlayer;
    static std::shared_ptr<Level> activeLevel;
    static std::shared_ptr<Camera> activeCamera;
    static std::shared_ptr<DebugInfo> debugInfo;
    static std::shared_ptr<EditSettings> editSettings;
    

    static std::atomic<unsigned int> audioGuid;
    static std::mutex audioLock;
    static GameState mainGameState;
    static InputSet inputSet;

public:

    static Logger<LogPolicy>* getLogger();
    static void setLoggingService(std::shared_ptr<Logger<LogPolicy>> _fileLogger);

    static Settings* getSettings();
    static void setSettings(std::shared_ptr<Settings> _settings);

    static SoundEngine* getAudio();
    static void setAudioEngine(std::shared_ptr<SoundEngine> _audio);

    static InputManager* getInputManager();
    static void setInputManager(std::shared_ptr<InputManager> _input);

    static void updateInput();
    static InputSet getInput();

    static RenderResource* getRenderResource();
    static void setRenderResource(std::shared_ptr<RenderResource> providedRenderResource);

    static BulletPhysics* getPhysics();
    static void setPhysics(BulletPhysics* providedPhysics);

    static CollisionDatabase* getCollisionDatabase();
    static void setCollisionDatabase(CollisionDatabase* providedCdb);

    static Player* getPlayer();
    static void setPlayer(std::shared_ptr<Player> _player);

    static Level* getActiveLevel();
    static void setActiveLevel(std::shared_ptr<Level> _level);

    static Camera* getActiveCamera();
    static void setActiveCamera(std::shared_ptr<Camera> _camera);

    static DebugInfo* getDebugInfo();
    static EditSettings* getEditSettings();

    static unsigned int getAudioGuid();

    static GameState getGameState();
    static void setGameState(GameState gameState);
};

namespace Helper {

    template<typename T>
    static bool keyInMap(std::unordered_map<std::string, T>& map, const std::string& key)
    {
        // Key is not present 
        if (map.find(key) == map.end())
            return false;

        return true;
    }

}