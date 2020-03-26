#include "serviceprovider.h"


std::shared_ptr<Logger<LogPolicy>>ServiceProvider::vsLogger = NULL;

std::shared_ptr<Settings>ServiceProvider::settings = NULL;
std::shared_ptr<SoundEngine>ServiceProvider::audio = NULL;
std::shared_ptr<InputManager>ServiceProvider::input = NULL;

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
