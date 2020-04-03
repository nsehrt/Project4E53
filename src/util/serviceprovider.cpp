#include "serviceprovider.h"


std::shared_ptr<Logger<LogPolicy>>ServiceProvider::vsLogger = nullptr;

std::shared_ptr<Settings>ServiceProvider::settings = nullptr;
std::shared_ptr<SoundEngine>ServiceProvider::audio = nullptr;
std::shared_ptr<InputManager>ServiceProvider::input = nullptr;
std::shared_ptr<RenderResource>ServiceProvider::renderResource = nullptr;
std::shared_ptr<Level>ServiceProvider::activeLevel = nullptr;
std::shared_ptr<Camera>ServiceProvider::activeCamera = nullptr;

std::atomic<unsigned int> ServiceProvider::audioGuid = 1;

std::mutex ServiceProvider::audioLock;



void ServiceProvider::setLoggingService(std::shared_ptr<Logger<LogPolicy>> providedFileLogger)
{
    vsLogger = providedFileLogger;
}

void ServiceProvider::setSettings(std::shared_ptr<Settings> providedSettings)
{
    settings = providedSettings;
}

void ServiceProvider::setAudioEngine(std::shared_ptr<SoundEngine> providedSound)
{
    audio = providedSound;
}

void ServiceProvider::setInputManager(std::shared_ptr<InputManager> providedInputManager)
{
    input = providedInputManager;
}

void ServiceProvider::setRenderResource(std::shared_ptr<RenderResource> providedRenderResource)
{
    renderResource = providedRenderResource;
}

void ServiceProvider::setActiveLevel(std::shared_ptr<Level> providedLevel)
{
    activeLevel = providedLevel;
}

void ServiceProvider::setActiveCamera(std::shared_ptr<Camera> providedCamera)
{
    activeCamera = providedCamera;
}
