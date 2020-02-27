#pragma once

#include "../extern/tinyxml2.h"
#include <string>
#include <memory>

#ifndef XMLCheckResult
    #define XMLCheckResult(a_eResult) if (a_eResult != XML_SUCCESS) { return false; }
#endif

#ifndef XMLCheckExist
    #define XMLCheckExist(eResult) if(eResult == nullptr){return false;}
#endif

struct DisplaySettings
{
    int GPUAdapter = 0;
    int Monitor = 0;
    int ResolutionWidth = 640;
    int ResolutionHeight = 480;
    float RefreshRate = 60.0;
    int VSync = 0;
    int WindowMode = 0;
    int BufferFrames = 3;
};

struct InputSettings
{
    int fill;
};

struct GraphicSettings
{
    int fill;
};

struct GameplaySettings
{
    int fill;
};

struct AudioSettings
{
    int fill;
};


struct Settings
{
    DisplaySettings displaySettings;
    InputSettings inputSettings;
    GraphicSettings graphicSettings;
    GameplaySettings gameplaySettings;
    AudioSettings audioSettings;
};

class SettingsLoader
{
public:
    SettingsLoader() {};
    ~SettingsLoader() {};

    bool loadSettings(const std::string& path);
    std::shared_ptr<Settings> get();

private:
    Settings settings;
};