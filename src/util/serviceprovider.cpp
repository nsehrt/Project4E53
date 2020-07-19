#include "serviceprovider.h"

std::shared_ptr<Logger<LogPolicy>>ServiceProvider::vsLogger = nullptr;

std::shared_ptr<Settings>ServiceProvider::settings = nullptr;
std::shared_ptr<SoundEngine>ServiceProvider::audio = nullptr;
std::shared_ptr<InputManager>ServiceProvider::input = nullptr;
std::shared_ptr<RenderResource>ServiceProvider::renderResource = nullptr;
std::shared_ptr<Level>ServiceProvider::activeLevel = nullptr;
std::shared_ptr<Camera>ServiceProvider::activeCamera = nullptr;
std::shared_ptr<DebugInfo>ServiceProvider::debugInfo = std::make_shared<DebugInfo>();
std::shared_ptr<EditSettings>ServiceProvider::editSettings = std::make_shared<EditSettings>();

std::atomic<unsigned int> ServiceProvider::audioGuid = 1;

std::mutex ServiceProvider::audioLock;

Logger<LogPolicy>* ServiceProvider::getLogger()
{
    return vsLogger.get();
}

void ServiceProvider::setLoggingService(std::shared_ptr<Logger<LogPolicy>> providedFileLogger)
{
    vsLogger = providedFileLogger;
}

Settings* ServiceProvider::getSettings()
{
    return settings.get();
}

void ServiceProvider::setSettings(std::shared_ptr<Settings> providedSettings)
{
    settings = providedSettings;
}

SoundEngine* ServiceProvider::getAudio()
{
    return audio.get();
}

void ServiceProvider::setAudioEngine(std::shared_ptr<SoundEngine> providedSound)
{
    audio = providedSound;
}

InputManager* ServiceProvider::getInputManager()
{
    return input.get();
}

void ServiceProvider::setInputManager(std::shared_ptr<InputManager> providedInputManager)
{
    input = providedInputManager;
}

RenderResource* ServiceProvider::getRenderResource()
{
    return renderResource.get();
}

void ServiceProvider::setRenderResource(std::shared_ptr<RenderResource> providedRenderResource)
{
    renderResource = providedRenderResource;
}

Level* ServiceProvider::getActiveLevel()
{
    return activeLevel.get();
}

void ServiceProvider::setActiveLevel(std::shared_ptr<Level> providedLevel)
{
    activeLevel = providedLevel;
}

Camera* ServiceProvider::getActiveCamera()
{
    return activeCamera.get();
}

void ServiceProvider::setActiveCamera(std::shared_ptr<Camera> providedCamera)
{
    activeCamera = providedCamera;
}

DebugInfo* ServiceProvider::getDebugInfo()
{
    return debugInfo.get();
}

EditSettings* ServiceProvider::getEditSettings()
{
    return editSettings.get();
}

unsigned int ServiceProvider::getAudioGuid()
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
