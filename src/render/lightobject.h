#pragma once

#include "../extern/json.hpp"
#include "../render/renderstructs.h"
#include "../render/renderresource.h"
#include "../util/serviceprovider.h"

using json = nlohmann::json;

enum class LightType
{
    Directional,
    Point,
    Spot
};

class LightObject
{

public:

    explicit LightObject(LightType type, const json& lightJson);

    std::string name;

    void updateHitbox();

    /*getter*/
    LightType getLightType() const
    {
        return lightType;
    }

    DirectX::XMFLOAT3 getStrength() const
    {
        return properties.Strength;
    }

    DirectX::XMFLOAT3 getDirection() const
    {
        return properties.Direction;
    }

    DirectX::XMFLOAT3 getPosition() const
    {
        return properties.Position;
    }

    float getFallOffStart() const
    {
        return properties.FalloffStart;
    }

    float getFallOffEnd() const
    {
        return properties.FalloffEnd;
    }

    float getSpotPower() const
    {
        return properties.SpotPower;
    }

    const Light& getLightProperties() const
    {
        return properties;
    }

    /*setter*/
    void setStrength(XMFLOAT3 str)
    {
        properties.Strength = str;
    }

    void setDirection(DirectX::XMFLOAT3 dir)
    {
        properties.Direction = dir;
    }

    void setPosition(DirectX::XMFLOAT3 pos)
    {
        properties.Position = pos;
        updateHitbox();
    }

    void setFallOffStart(float falloff)
    {
        properties.FalloffStart = falloff;
    }

    void setFallOffEnd(float falloff)
    {
        properties.FalloffEnd = falloff;
    }

    void setSpotPower(float pow)
    {
        properties.SpotPower = pow;
    }


private:

    
    LightType lightType = LightType::Directional;

    Light properties = {};

    DirectX::BoundingSphere bounding;

    bool exists(const nlohmann::json& j, const std::string& key)
    {
        return j.find(key) != j.end();
    }

};

/*
struct Light
{
    DirectX::XMFLOAT3 Strength = { 0.0f, 0.0f, 0.0f };
    float FalloffStart = 1.0f;                          // point/spot light only
    DirectX::XMFLOAT3 Direction = { 0.0f, -1.0f, 0.0f };// directional/spot light only
    float FalloffEnd = 10.0f;                           // point/spot light only
    DirectX::XMFLOAT3 Position = { 0.0f, 0.0f, 0.0f };  // point/spot light only
    float SpotPower = 64.0f;                            // spot light only
};
*/