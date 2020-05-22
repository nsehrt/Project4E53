#include "particlesystem.h"

void ParticleSystem::init(const json& particleJson)
{
    name = particleJson["Name"];

    position.x = particleJson["Position"][0];
    position.y = particleJson["Position"][1];
    position.z = particleJson["Position"][2];

    particleSize.x = particleJson["Size"][0];
    particleSize.y = particleJson["Size"][1];

    particleCount = particleJson["ParticleCount"];
    particleCount = MathHelper::minH(particleCount, MAX_PARTICLE);

    std::string type = particleJson["Type"];

    if (type == "Smoke")
    {
        particleSystemType = ParticleSystemType::Smoke;
    }
    else if (type == "Fire")
    {
        particleSystemType = ParticleSystemType::Fire;
    }

    textureName = particleJson["Texture"];
    textureName = particleJson["Material"];

    /*create mesh*/
    GeometryGenerator geoGen;

    std::vector<std::uint16_t> indices(particleCount);
    mParticleVertices.resize(particleCount);

    for (size_t i = 0; i < particleCount; i++)
    {
        mParticleVertices[i].Pos.x = position.x;
        mParticleVertices[i].Pos.y = position.y;
        mParticleVertices[i].Pos.z = position.z;

        mParticleVertices[i].Size.x = particleSize.x;
        mParticleVertices[i].Size.y = particleSize.y;

        mParticleVertices[i].Visible = 0;
    }

    for (int i = 0; i < indices.size(); i++)
    {
        indices[i] = i;
    }

    UINT vbByteSize = (UINT)mParticleVertices.size() * sizeof(ParticleVertex);
    UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

    auto geo = std::make_unique<Mesh>();

    geo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(renderResource->device,
                                                        renderResource->cmdList, mParticleVertices.data(), vbByteSize, geo->VertexBufferUploader);

    geo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(renderResource->device,
                                                       renderResource->cmdList, indices.data(), ibByteSize, geo->IndexBufferUploader);

    geo->VertexByteStride = sizeof(ParticleVertex);
    geo->VertexBufferByteSize = vbByteSize;
    geo->IndexFormat = DXGI_FORMAT_R16_UINT;
    geo->IndexBufferByteSize = ibByteSize;
    geo->IndexCount = (UINT)indices.size();

    particleSystemModel = std::make_unique<Model>();

    particleSystemModel->name = "PARTICLE";
    particleSystemModel->group = "particle";
    particleSystemModel->meshes.push_back(std::move(geo));


    updateTime = MathHelper::getRandomFloat(0.0f, updFixedTime);
}

void ParticleSystem::update(const GameTime& gt)
{
}
