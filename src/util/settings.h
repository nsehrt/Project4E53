#pragma once

#include "../extern/tinyxml2.h"
#include <string>
#include <memory>

#ifndef XMLCheckExist
    #define XMLCheckExist(eResult) if(eResult == nullptr){return false;}
#endif

struct DisplaySettings
{
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

struct Misc
{
    std::string AdapterName;


};


struct Settings
{
    DisplaySettings displaySettings;
    InputSettings inputSettings;
    GraphicSettings graphicSettings;
    GameplaySettings gameplaySettings;
    AudioSettings audioSettings;
    Misc miscSettings;
};

class SettingsLoader
{
private:
    Settings settings;

public:
    SettingsLoader() { settings.miscSettings.AdapterName = ""; };
    ~SettingsLoader() {};

    bool loadSettings(const std::string& path);
    std::shared_ptr<Settings> get();

private:
    bool setSetting(tinyxml2::XMLElement* r, const char* id, int* target);
    bool setSetting(tinyxml2::XMLElement* r, const char* id, float* target);
};