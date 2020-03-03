#pragma once

#include "log.h"
#include "settings.h"
#include "../input/InputManager.h"

/*forward declaration*/
class InputManager;


class ServiceProvider
{
private:
    static std::shared_ptr<Logger<FileLogPolicy>> fileLogger;
    static std::shared_ptr<Logger<CLILogPolicy>> cliLogger;
    static std::shared_ptr<Logger<VSLogPolicy>> vsLogger;

    static std::shared_ptr<Settings> settings;

    static std::shared_ptr<InputManager> input;

public:
    static Logger<FileLogPolicy>* getFileLogger() { return fileLogger.get(); }
    static void setFileLoggingService(std::shared_ptr<Logger<FileLogPolicy>> providedFileLogger);

    static Logger<CLILogPolicy>* getCLILogger() { return cliLogger.get(); }
    static void setCLILoggingService(std::shared_ptr<Logger<CLILogPolicy>> providedFileLogger);

    static Logger<VSLogPolicy>* getVSLogger() { return vsLogger.get(); }
    static void setVSLoggingService(std::shared_ptr<Logger<VSLogPolicy>> providedFileLogger);


    static Settings* getSettings() { return settings.get(); }
    static void setSettings(std::shared_ptr<Settings> providedSettings);

    static InputManager* getInputManager() { return input.get(); }
    static void setInputManager(std::shared_ptr<InputManager> providedInputManager);
};