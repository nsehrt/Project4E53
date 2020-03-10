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

    /*load gameplay settings*/
    XMLElement* pGameplay = pRoot->FirstChildElement("Gameplay");
    XMLCheckExist(pGameplay);

    /*load graphic settings*/
    XMLElement* pGraphic = pRoot->FirstChildElement("Graphic");
    XMLCheckExist(pGraphic);

    if (!setSetting(pGraphic, "NumFrameResources", &settings.graphicSettings.numFrameResources))
        return false;

    /*load input settings*/
    XMLElement* pInput = pRoot->FirstChildElement("Input");
    XMLCheckExist(pInput);


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