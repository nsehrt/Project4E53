#pragma once

/*forward declaration*/
class InputManager;
class SoundEngine;
class RenderResource;
class Level;
class Camera;
class GameObject;
class ShadowMap;
class LightObject;
class EditModeHUD;
struct EditSettings;

#include "log.h"
#include "settings.h"
#include "../input/InputManager.h"
#include "../audio/soundengine.h"
#include "../render/renderresource.h"
#include "../core/level.h"
#include "../core/camera.h"
#include "../util/debuginfo.h"
#include "../core/editmode.h"

class ServiceProvider
{
private:

    static std::shared_ptr<Logger<LogPolicy>> vsLogger;

    static std::shared_ptr<Settings> settings;
    static std::shared_ptr<SoundEngine> audio;
    static std::shared_ptr<InputManager> input;
    static std::shared_ptr<RenderResource> renderResource;

    static std::shared_ptr<Level> activeLevel;
    static std::shared_ptr<Camera> activeCamera;
    static std::shared_ptr<DebugInfo> debugInfo;
    static std::shared_ptr<EditSettings> editSettings;

    static std::atomic<unsigned int> audioGuid;
    static std::mutex audioLock;

public:

    static Logger<LogPolicy>* getLogger() { return vsLogger.get(); }
    static void setLoggingService(std::shared_ptr<Logger<LogPolicy>> providedFileLogger);

    static Settings* getSettings() { return settings.get(); }
    static void setSettings(std::shared_ptr<Settings> providedSettings);

    static SoundEngine* getAudio() { return audio.get(); }
    static void setAudioEngine(std::shared_ptr<SoundEngine> providedAudio);

    static InputManager* getInputManager() { return input.get(); }
    static void setInputManager(std::shared_ptr<InputManager> providedInputManager);

    static RenderResource* getRenderResource() { return renderResource.get(); }
    static void setRenderResource(std::shared_ptr<RenderResource> providedRenderResource);

    static Level* getActiveLevel() { return activeLevel.get(); };
    static void setActiveLevel(std::shared_ptr<Level> providedLevel);

    static Camera* getActiveCamera() { return activeCamera.get(); };
    static void setActiveCamera(std::shared_ptr<Camera> providedCamera);

    static DebugInfo* getDebugInfo() { return debugInfo.get(); };

    static EditSettings* getEditSettings() { return editSettings.get(); };

    static unsigned int getAudioGuid()
    {
        std::lock_guard<std::mutex> lock(ServiceProvider::audioLock);

        unsigned int ret = audioGuid.load();
        audioGuid++;

        if (audioGuid == 0)
        {
            audioGuid++;
        }

        return ret;
    }
};