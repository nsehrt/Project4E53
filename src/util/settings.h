#pragma once

#include "../extern/tinyxml2.h"
#include <string>
#include <memory>

#ifndef XMLCheckExist
    #define XMLCheckExist(eResult) if(eResult == nullptr){return false;}
#endif

static int shadowMapSizes[] = { 512, 1024,2048,4096,8192 };

struct DisplaySettings
{
    int Monitor = 0;
    int ResolutionWidth = 640;
    int ResolutionHeight = 480;
    float RefreshRate = 60.0;
    int FOV = 90;
    int VSync = 0;
    int WindowMode = 0;
    int BufferFrames = 3;
};

struct InputSettings
{
    int InvertYAxis = 0;
    float Sensitivity = 1.0f;
    float FPSCameraSpeed = 1.0f;
};

struct GraphicSettings
{
    bool WireFrameMode = false;
    int numFrameResources = 3;
    int AnisotropicFiltering = 16;
    int ShadowEnabled = 1;
    int ShadowQuality = 4;
};

struct GameplaySettings
{
    int fill = 0;
};

struct AudioSettings
{
    float MasterVolume = 1.0f;
    float EffectVolume = 1.0f;
    float MusicVolume = 1.0f;
};

struct Misc
{
    std::string AdapterName = "";
    int DebugEnabled = false;

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
    SettingsLoader() {};
    ~SettingsLoader() {};

    bool loadSettings(const std::string& path);
    std::shared_ptr<Settings> get();

private:
    bool setSetting(tinyxml2::XMLElement* r, const char* id, int* target);
    bool setSetting(tinyxml2::XMLElement* r, const char* id, float* target);
};