#include "../render/renderresource.h"

class ParticleSystem
{
public:
    explicit ParticleSystem(RenderResource* r)
    {
        renderResource = r;
    }

    void init(const json& particleJson);
    void update(const GameTime& gt);

    Model* getModel() const
    {
        return particleSystemModel.get();
    }

    enum class ParticleSystemType : int
    {
        Fire,
        Smoke
    };

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
    std::string textureName = "default";
    std::string materialName = "default";

    float updateTime = 0.0f;
    const float updFixedTime = 1.0f / 60.0f;
};