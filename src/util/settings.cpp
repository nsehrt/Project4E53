#include "settings.h"

using namespace tinyxml2;

bool SettingsLoader::loadSettings(const std::string& path)
{
    XMLDocument xmlDoc;

    XMLError eResult = xmlDoc.LoadFile(path.c_str());

    if (eResult != XML_SUCCESS)
        return false;

    XMLNode* pRoot = xmlDoc.FirstChildElement("root");

    XMLCheckExist(pRoot);

    /*load audio settings*/
    XMLElement* pAudio = pRoot->FirstChildElement("Audio");
    XMLCheckExist(pAudio);

    if (!setSetting(pAudio, "MasterVolume", &settings.audioSettings.MasterVolume))
        return false;

    if (!setSetting(pAudio, "EffectVolume", &settings.audioSettings.EffectVolume))
        return false;

    if (!setSetting(pAudio, "MusicVolume", &settings.audioSettings.MusicVolume))
        return false;

    /*load display settings*/
    XMLElement* pDisplay = pRoot->FirstChildElement("Display");
    XMLCheckExist(pDisplay);

    if (!setSetting(pDisplay, "Monitor", &settings.displaySettings.Monitor))
        return false;

    if (!setSetting(pDisplay, "ResolutionWidth", &settings.displaySettings.ResolutionWidth))
        return false;

    if (!setSetting(pDisplay, "ResolutionHeight", &settings.displaySettings.ResolutionHeight))
        return false;

    if (!setSetting(pDisplay, "VSync", &settings.displaySettings.VSync))
        return false;

    if (settings.displaySettings.VSync < 0 || settings.displaySettings.VSync > 4)
    {
        settings.displaySettings.VSync = 0;
    }

    if (!setSetting(pDisplay, "WindowMode", &settings.displaySettings.WindowMode))
        return false;

    if (!setSetting(pDisplay, "BufferFrames", &settings.displaySettings.BufferFrames))
        return false;

    if (!setSetting(pDisplay, "RefreshRate", &settings.displaySettings.RefreshRate))
        return false;

    if (!setSetting(pDisplay, "FOV", &settings.displaySettings.FOV))
        return false;

    if (settings.displaySettings.FOV < 30)
        settings.displaySettings.FOV = 30;

    if (settings.displaySettings.FOV > 240)
        settings.displaySettings.FOV = 240;

    /*load gameplay settings*/
    XMLElement* pGameplay = pRoot->FirstChildElement("Gameplay");
    XMLCheckExist(pGameplay);

    /*load graphic settings*/
    XMLElement* pGraphic = pRoot->FirstChildElement("Graphic");
    XMLCheckExist(pGraphic);

    if (!setSetting(pGraphic, "NumFrameResources", &settings.graphicSettings.numFrameResources))
        return false;

    if (!setSetting(pGraphic, "AnisotropicFiltering", &settings.graphicSettings.AnisotropicFiltering))
        return false;

    if (settings.graphicSettings.AnisotropicFiltering > 16)
    {
        settings.graphicSettings.AnisotropicFiltering = 16;
    }
    else if (settings.graphicSettings.AnisotropicFiltering < 0)
    {
        settings.graphicSettings.AnisotropicFiltering = 0;
    }

    if (!setSetting(pGraphic, "ShadowEnabled", &settings.graphicSettings.ShadowEnabled))
        return false;

    if (settings.graphicSettings.ShadowEnabled != 0)
    {
        settings.graphicSettings.ShadowEnabled = 1;
    }

    if (!setSetting(pGraphic, "SobelFilter", &settings.graphicSettings.SobelFilter))
        return false;

    if (settings.graphicSettings.SobelFilter != 0)
    {
        settings.graphicSettings.SobelFilter = 1;
    }

    if (!setSetting(pGraphic, "ShadowQuality", &settings.graphicSettings.ShadowQuality))
        return false;

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
    XMLElement* pInput = pRoot->FirstChildElement("Input");
    XMLCheckExist(pInput);

    if (!setSetting(pInput, "InvertYAxis", &settings.inputSettings.InvertYAxis))
        return false;

    if (settings.inputSettings.InvertYAxis != 0)
    {
        settings.inputSettings.InvertYAxis = 1;
    }

    if (!setSetting(pInput, "Sensitivity", &settings.inputSettings.Sensitivity))
        return false;

    if (!setSetting(pInput, "FPSCameraSpeed", &settings.inputSettings.FPSCameraSpeed))
        return false;

    /*load misc settings*/
    XMLElement* pMisc = pRoot->FirstChildElement("Misc");
    XMLCheckExist(pMisc);

    if (!setSetting(pMisc, "DebugEnabled", &settings.miscSettings.DebugEnabled))
        return false;

    if (settings.miscSettings.DebugEnabled != 0)
    {
        settings.miscSettings.DebugEnabled = 1;
    }

    if (!setSetting(pMisc, "DebugQuadEnabled", &settings.miscSettings.DebugQuadEnabled))
        return false;

    if (settings.miscSettings.DebugQuadEnabled != 0)
    {
        settings.miscSettings.DebugQuadEnabled = 1;
    }

    if (!setSetting(pMisc, "EditModeEnabled", &settings.miscSettings.EditModeEnabled))
        return false;

    if (settings.miscSettings.EditModeEnabled != 0)
    {
        settings.miscSettings.EditModeEnabled = 1;
    }

    return true;
}

std::shared_ptr<Settings> SettingsLoader::get()
{
    return std::make_shared<Settings>(settings);
}

bool SettingsLoader::setSetting(XMLElement* r, const char* id, int* target)
{
    XMLElement* value = r->FirstChildElement(id);
    XMLError eResult;
    XMLCheckExist(value);
    eResult = value->QueryIntText(target);

    if (eResult != XML_SUCCESS)
    {
        return false;
    }

    return true;
}

bool SettingsLoader::setSetting(XMLElement* r, const char* id, float* target)
{
    XMLElement* value = r->FirstChildElement(id);
    XMLError eResult;
    XMLCheckExist(value);
    eResult = value->QueryFloatText(target);

    if (eResult != XML_SUCCESS)
    {
        return false;
    }

    return true;
}