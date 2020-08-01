#include "settings.h"
#include "../extern/json.hpp"
#include "../util/serviceprovider.h"

using json = nlohmann::json;

bool SettingsLoader::loadSettings(const std::string& path)
{
    /*open and parse settings file*/
    std::ifstream settingsHandle(path);

    if (!settingsHandle.is_open())
    {
        LOG(Severity::Critical, "Unable to open the settings file!");
        return false;
    }

    json settingsJson;

    try
    {
        settingsJson = json::parse(settingsHandle);
    }
    catch (nlohmann::detail::parse_error)
    {
        LOG(Severity::Critical, "Error while parsing settings file! Check JSON validity!")
            return false;
    }
    catch (...)
    {
        LOG(Severity::Critical, "Unknown error with settings file!")
            return false;
    }

    settingsHandle.close();

    /*load audio settings*/
    settings.audioSettings.MasterVolume = settingsJson["Audio"]["MasterVolume"];
    settings.audioSettings.EffectVolume = settingsJson["Audio"]["EffectVolume"];
    settings.audioSettings.MusicVolume = settingsJson["Audio"]["MusicVolume"];

    /*load display settings*/
    settings.displaySettings.Monitor = settingsJson["Display"]["Monitor"];
    settings.displaySettings.ResolutionWidth = settingsJson["Display"]["ResolutionWidth"];
    settings.displaySettings.ResolutionHeight = settingsJson["Display"]["ResolutionHeight"];
    settings.displaySettings.VSync = settingsJson["Display"]["VSync"];
    settings.displaySettings.WindowMode = settingsJson["Display"]["WindowMode"];
    settings.displaySettings.RefreshRate = settingsJson["Display"]["RefreshRate"];
    settings.displaySettings.FOV = settingsJson["Display"]["FOV"];

    if (settings.displaySettings.VSync < 0 || settings.displaySettings.VSync > 4)
    {
        settings.displaySettings.VSync = 0;
    }

    if (settings.displaySettings.FOV < 30)
        settings.displaySettings.FOV = 30;

    if (settings.displaySettings.FOV > 240)
        settings.displaySettings.FOV = 240;

    /*load gameplay settings*/


    /*---*/


    /*load graphic settings*/

    settings.graphicSettings.numFrameResources = settingsJson["Graphic"]["NumFrameResources"];
    settings.graphicSettings.AnisotropicFiltering = settingsJson["Graphic"]["AnisotropicFiltering"];
    settings.graphicSettings.ShadowEnabled = settingsJson["Graphic"]["ShadowEnabled"];
    settings.graphicSettings.SobelFilter = settingsJson["Graphic"]["SobelFilter"];
    settings.graphicSettings.ShadowQuality = settingsJson["Graphic"]["ShadowQuality"];



    if (settings.graphicSettings.AnisotropicFiltering > 16)
    {
        settings.graphicSettings.AnisotropicFiltering = 16;
    }
    else if (settings.graphicSettings.AnisotropicFiltering < 0)
    {
        settings.graphicSettings.AnisotropicFiltering = 0;
    }

    if (settings.graphicSettings.ShadowEnabled != 0)
    {
        settings.graphicSettings.ShadowEnabled = 1;
    }

    if (settings.graphicSettings.SobelFilter != 0)
    {
        settings.graphicSettings.SobelFilter = 1;
    }

    if (settings.graphicSettings.ShadowQuality > 3)
    {
        settings.graphicSettings.ShadowQuality = 3;
    }
    else if (settings.graphicSettings.ShadowQuality < 0)
    {
        settings.graphicSettings.ShadowQuality = 0;
    }

    settings.graphicSettings.ShadowQuality = shadowMapSizes[settings.graphicSettings.ShadowQuality];


    /*load input settings*/

    settings.inputSettings.InvertYAxis = settingsJson["Input"]["InvertYAxis"];
    settings.inputSettings.Sensitivity = settingsJson["Input"]["Sensitivity"];
    settings.inputSettings.FPSCameraSpeed = settingsJson["Input"]["FPSCameraSpeed"];

    if (settings.inputSettings.InvertYAxis != 0)
    {
        settings.inputSettings.InvertYAxis = 1;
    }

    /*load misc settings*/
    settings.miscSettings.DebugEnabled = settingsJson["Misc"]["DebugEnabled"];
    settings.miscSettings.DebugQuadEnabled = settingsJson["Misc"]["DebugQuadEnabled"];
    settings.miscSettings.EditModeEnabled = settingsJson["Misc"]["EditModeEnabled"];
    settings.miscSettings.DrawFPSEnabled = settingsJson["Misc"]["DrawFPSEnabled"];

    if (settings.miscSettings.DebugEnabled != 0)
    {
        settings.miscSettings.DebugEnabled = 1;
    }

    if (settings.miscSettings.DebugQuadEnabled != 0)
    {
        settings.miscSettings.DebugQuadEnabled = 1;
    }

    if (settings.miscSettings.EditModeEnabled != 0)
    {
        settings.miscSettings.EditModeEnabled = 1;
    }

    if (settings.miscSettings.DrawFPSEnabled != 0)
    {
        settings.miscSettings.DrawFPSEnabled = 1;
    }

    return true;
}

std::shared_ptr<Settings> SettingsLoader::get()
{
    return std::make_shared<Settings>(settings);
}