#pragma once

#include "log.h"
#include "settings.h"
#include "../input/InputManager.h"
#include "../audio/soundengine.h"

/*forward declaration*/
class InputManager;
class SoundEngine;

class ServiceProvider
{
private:
    //static std::shared_ptr<Logger<FileLogPolicy>> fileLogger;
    //static std::shared_ptr<Logger<CLILogPolicy>> cliLogger;
    static std::shared_ptr<Logger<LogPolicy>> vsLogger;

    static std::shared_ptr<Settings> settings;
    static std::shared_ptr<SoundEngine> audio;
    static std::shared_ptr<InputManager> input;

    static std::atomic<unsigned int> audioGuid;
    static std::mutex audioLock;

public:
    //static Logger<FileLogPolicy>* getFileLogger() { return fileLogger.get(); }
    //static void setFileLoggingService(std::shared_ptr<Logger<FileLogPolicy>> providedFileLogger);

    //static Logger<CLILogPolicy>* getCLILogger() { return cliLogger.get(); }
    //static void setCLILoggingService(std::shared_ptr<Logger<CLILogPolicy>> providedFileLogger);

    static Logger<LogPolicy>* getLogger() { return vsLogger.get(); }
    static void setLoggingService(std::shared_ptr<Logger<LogPolicy>> providedFileLogger);

    static Settings* getSettings() { return settings.get(); }
    static void setSettings(std::shared_ptr<Settings> providedSettings);

    static SoundEngine* getAudio() { return audio.get(); }
    static void setAudioEngine(std::shared_ptr<SoundEngine> providedAudio);

    static InputManager* getInputManager() { return input.get(); }
    static void setInputManager(std::shared_ptr<InputManager> providedInputManager);

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