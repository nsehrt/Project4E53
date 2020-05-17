#include "../render/renderresource.h"

class Water
{

public:
    explicit Water(RenderResource* r, const json& waterJson);


    void update(const GameTime& gt);

    std::string getName()
    {
        return materialName;
    }

    bool isSaved = false;

    void addPropertiesToJson(json& wJson);

private:

    void border(float& x);

    float updateTime = 0.0f;
    Material* material = nullptr;
    std::string materialName = "";

    DirectX::XMFLOAT3 displacement1Scale = { 1.0f,1.0f,1.0f };
    DirectX::XMFLOAT3 displacement2Scale = { 1.0f,1.0f,1.0f };

    DirectX::XMFLOAT2 materialTranslation = { 0.0f,0.0f };
    DirectX::XMFLOAT2 displacement1Translation = { 0.0f,0.0f };
    DirectX::XMFLOAT2 displacement2Translation = { 0.0f,0.0f };

    DirectX::XMFLOAT3 matScale = { 1.0f,1.0f,1.0f };

    const float updFixedTime = 1.0f / 60.0f;


    float getRandomFloat(float min, float max)
    {
        float random = ((float)rand()) / (float)RAND_MAX;

        float range = max - min;
        return (random * range) + min;
    }

};