#include "..\core\lightobject.h"

LightObject::LightObject(LightType type, const json& lightJson)
{
    lightType = type;

    name = lightJson["Name"];

    if (!exists(lightJson, "Strength"))
    {
        LOG(Severity::Warning, "LightObject " << name << " misses Strength property!");
        return;
    }

    properties.Strength.x = lightJson["Strength"][0];
    properties.Strength.y = lightJson["Strength"][1];
    properties.Strength.z = lightJson["Strength"][2];

    /*directional light*/
    if (lightType == LightType::Directional)
    {
        if (!exists(lightJson, "Direction"))
        {
            LOG(Severity::Warning, "LightObject " << name << " misses Direction property!");
            return;
        }

        properties.Direction.x = lightJson["Direction"][0];
        properties.Direction.y = lightJson["Direction"][1];
        properties.Direction.z = lightJson["Direction"][2];
    }
    /*point light*/
    else if (lightType == LightType::Point)
    {
        if (!exists(lightJson, "Position"))
        {
            LOG(Severity::Warning, "LightObject " << name << " misses Position property!");
            return;
        }

        properties.Position.x = lightJson["Position"][0];
        properties.Position.y = lightJson["Position"][1];
        properties.Position.z = lightJson["Position"][2];

        if (!exists(lightJson, "FallOffStart"))
        {
            LOG(Severity::Warning, "LightObject " << name << " misses FallOffStart property!");
            return;
        }

        properties.FalloffStart = lightJson["FallOffStart"];

        if (!exists(lightJson, "FallOffEnd"))
        {
            LOG(Severity::Warning, "LightObject " << name << " misses FallOffEnd property!");
            return;
        }

        properties.FalloffEnd = lightJson["FallOffEnd"];
    }
    /*spot light*/
    else
    {
        if (!exists(lightJson, "Position"))
        {
            LOG(Severity::Warning, "LightObject " << name << " misses Position property!");
            return;
        }

        properties.Position.x = lightJson["Position"][0];
        properties.Position.y = lightJson["Position"][1];
        properties.Position.z = lightJson["Position"][2];

        if (!exists(lightJson, "Direction"))
        {
            LOG(Severity::Warning, "LightObject " << name << " misses Direction property!");
            return;
        }

        properties.Direction.x = lightJson["Direction"][0];
        properties.Direction.y = lightJson["Direction"][1];
        properties.Direction.z = lightJson["Direction"][2];

        if (!exists(lightJson, "FallOffStart"))
        {
            LOG(Severity::Warning, "LightObject " << name << " misses FallOffStart property!");
            return;
        }

        properties.FalloffStart = lightJson["FallOffStart"];

        if (!exists(lightJson, "FallOffEnd"))
        {
            LOG(Severity::Warning, "LightObject " << name << " misses FallOffEnd property!");
            return;
        }

        properties.FalloffEnd = lightJson["FallOffEnd"];

        if (!exists(lightJson, "SpotPower"))
        {
            LOG(Severity::Warning, "LightObject " << name << " misses SpotPower property!");
            return;
        }

        properties.SpotPower = lightJson["SpotPower"];
    }

    updateHitbox();
}

LightObject::LightObject()
{
    properties = Light();
}

void LightObject::updateHitbox()
{
    bounding.Center = getPosition();
    bounding.Radius = 0.5f;
}