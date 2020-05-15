#include "../render/renderresource.h"
#include "../core/terrain.h"

class Grass
{
public:
    explicit Grass(RenderResource* r)
    {
        renderResource = r;
    }

    void create(const json& grassJson, Terrain* terrain);
    json toJson();

    Model* getPatchModel()
    {
        return grassPatchModel.get();
    }

    std::string getMaterialName()
    {
        return materialName;
    }

    DirectX::XMFLOAT3 getPosition()
    {
        return position;
    }

    DirectX::XMINT2 getDensity()
    {
        return density;
    }

    DirectX::XMFLOAT2 getSize()
    {
        return size;
    }

    std::string getName()
    {
        return name;
    }

private:
    RenderResource* renderResource;
    std::unique_ptr<Model> grassPatchModel = nullptr;
    DirectX::XMFLOAT3 position = { 0.0f,0.0f,0.0f };
    DirectX::XMFLOAT2 size = { 10.0f,10.0f };
    DirectX::XMFLOAT2 quadSize = { 1.0f,1.0f };
    float sizeVariation = 0.0f;
    DirectX::XMINT2 density = { 32,32 };
    std::string materialName;
    std::string name;

};