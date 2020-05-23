#include "../render/renderresource.h"

enum class ParticleSystemType : int
{
    Fire,
    Smoke
};

class ParticleSystem
{
public:
    explicit ParticleSystem(RenderResource* r)
    {
        renderResource = r;
    }

    void init(const json& particleJson);
    void update(const GameTime& gt);

    json toJson();

    Model* getModel() const
    {
        return particleSystemModel.get();
    }

    std::string getMaterialName() const
    {
        return materialName;
    }

    DirectX::XMFLOAT3 getPosition() const
    {
        return position;
    }

    ParticleSystemType getType() const
    {
        return particleSystemType;
    }

    static const UINT MAX_PARTICLE = 1000;

private:
    RenderResource* renderResource;

    std::string name = "particlesystem";
    std::unique_ptr<Model> particleSystemModel = nullptr;
    std::vector<ParticleVertex> mParticleVertices;
    ParticleSystemType particleSystemType = ParticleSystemType::Fire;

    UINT particleCount = 100;
    DirectX::XMFLOAT3 position = { 0.0f,0.0f,0.0f };
    DirectX::XMFLOAT2 particleSize = { 1.0f,1.0f };
    std::string materialName = "default";
    float spawnNewParticleTime = 0.005f;
    float maxAge = 1.0f;

    float updateTime = 0.0f;
    const float updFixedTime = 1.0f / 60.0f;
};