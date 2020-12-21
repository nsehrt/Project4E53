#pragma once

#include "../render/renderresource.h"

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
    explicit LightObject();

    std::string name;

    void updateHitbox();

    json toJson();

    /*getter*/
    LightType getLightType() const
    {
        return lightType;
    }

    DirectX::XMFLOAT3 getStrength() const
    {
        return properties.Strength;
    }

    DirectX::XMFLOAT3& getDirection()
    {
        return properties.Direction;
    }

    DirectX::XMFLOAT3& getPosition()
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
    void setStrength(DirectX::XMFLOAT3 str)
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