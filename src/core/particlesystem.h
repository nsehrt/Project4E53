#include "../render/renderresource.h"

enum class ParticleSystemType : int
{
    Fire,
    Smoke
};

class ParticleSystem
{
public:
    explicit ParticleSystem(RenderResource* r, UINT index)
    {
        renderResource = r;
        vbIndex = index;
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

    DirectX::XMFLOAT3 getRoughDimensions() const
    {
        return { particleSize.x * 2.0f, particleSize.y * 8.0f, particleSize.x * 2.0f };
    }

    static const UINT MAX_PARTICLE = 1000;

private:
    RenderResource* renderResource;

    std::string name = "particlesystem";
    std::unique_ptr<Model> particleSystemModel = nullptr;
    std::vector<ParticleVertex> mParticleVertices;
    Microsoft::WRL::ComPtr<ID3D12Resource> holder;
    ParticleSystemType particleSystemType = ParticleSystemType::Fire;

    UINT particleCount = 100;
    DirectX::XMFLOAT3 position = { 0.0f,0.0f,0.0f };
    DirectX::XMFLOAT2 particleSize = { 1.0f,1.0f };
    DirectX::XMFLOAT3 directionMultiplier = { 1.0f,1.0f,1.0f };
    std::string materialName = "default";

    float spawnNewParticleTime = 0.005f;
    float maxAge = 1.0f;
    float updateTime = 0.0f;
    float particleSpawnTimeSafe = 0.0f;
    const float updFixedTime = 1.0f / 60.0f;

    UINT vbIndex = 0;
};