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

    /*load display settings*/
    XMLElement* pDisplay = pRoot->FirstChildElement("Display");
    XMLCheckExist(pDisplay);

    XMLElement* pValue = pDisplay->FirstChildElement("GPUAdapter");
    XMLCheckExist(pValue);
    eResult = pValue->QueryIntText(&settings.displaySettings.GPUAdapter);
    XMLCheckResult(eResult);

    pValue = pDisplay->FirstChildElement("Monitor");
    XMLCheckExist(pValue);
    eResult = pValue->QueryIntText(&settings.displaySettings.Monitor);
    XMLCheckResult(eResult);

    pValue = pDisplay->FirstChildElement("ResolutionWidth");
    XMLCheckExist(pValue);
    eResult = pValue->QueryIntText(&settings.displaySettings.ResolutionWidth);
    XMLCheckResult(eResult);

    pValue = pDisplay->FirstChildElement("ResolutionHeight");
    XMLCheckExist(pValue);
    eResult = pValue->QueryIntText(&settings.displaySettings.ResolutionHeight);
    XMLCheckResult(eResult);

    pValue = pDisplay->FirstChildElement("VSync");
    XMLCheckExist(pValue);
    eResult = pValue->QueryIntText(&settings.displaySettings.VSync);
    XMLCheckResult(eResult);

    pValue = pDisplay->FirstChildElement("WindowMode");
    XMLCheckExist(pValue);
    eResult = pValue->QueryIntText(&settings.displaySettings.WindowMode);
    XMLCheckResult(eResult);

    pValue = pDisplay->FirstChildElement("BufferFrames");
    XMLCheckExist(pValue);
    eResult = pValue->QueryIntText(&settings.displaySettings.BufferFrames);
    XMLCheckResult(eResult);

    pValue = pDisplay->FirstChildElement("RefreshRate");
    XMLCheckExist(pValue);
    eResult = pValue->QueryFloatText(&settings.displaySettings.RefreshRate);
    XMLCheckResult(eResult);

    /*load gameplay settings*/
    XMLElement* pGameplay = pRoot->FirstChildElement("Gameplay");
    XMLCheckExist(pGameplay);

    /*load graphic settings*/
    XMLElement* pGraphic = pRoot->FirstChildElement("Graphic");
    XMLCheckExist(pGraphic);

    /*load input settings*/
    XMLElement* pInput = pRoot->FirstChildElement("Input");
    XMLCheckExist(pInput);


    return true;
}

std::shared_ptr<Settings> SettingsLoader::get()
{
    return std::make_shared<Settings>(settings);
}