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

    materialName = particleJson["Material"];

    /*create mesh*/
    GeometryGenerator geoGen;

    std::vector<std::uint16_t> indices(particleCount);
    mParticleVertices.resize(particleCount);

    for (size_t i = 0; i < particleCount; i++)
    {
        mParticleVertices[i].Pos = { 0.0f,0.0f,0.0f };
        mParticleVertices[i].Velocity = { 0.0f,0.0f,0.0f };
        mParticleVertices[i].Size = particleSize;

        mParticleVertices[i].Visible = 1;
        mParticleVertices[i].Age = 0.0f;
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
    updateTime += gt.DeltaTime();

    if (updateTime >= updFixedTime)
    {
        UINT newParticles = updateTime / spawnNewParticleTime;

        for (UINT i = 0; i < mParticleVertices.size(); i++)
        {

            if (mParticleVertices[i].Visible)
            {

                /*update/kill existing particle*/
                mParticleVertices[i].Age += updFixedTime;

                if (mParticleVertices[i].Age > maxAge)
                {
                    mParticleVertices[i].Age = 0.0f;
                    mParticleVertices[i].Visible = 0;
                }


            }
            else
            {
                /*create new particle*/
                if (newParticles > 0)
                {

                    mParticleVertices[i].Visible = 1;
                    mParticleVertices[i].Age = newParticles * spawnNewParticleTime;
                    XMStoreFloat3(&mParticleVertices[i].Velocity, XMVectorMultiply(MathHelper::randUnitVec3(), XMVectorSet(2.0f, 4.0f,2.0f,0.0f)));

                    newParticles--;
                }

            }


        }

        /*copy to gpu*/



        updateTime -= updFixedTime;
    }

}

json ParticleSystem::toJson()
{
    json jElement;

    jElement["Name"] = name;

    jElement["Position"][0] = getPosition().x;
    jElement["Position"][1] = getPosition().y;
    jElement["Position"][2] = getPosition().z;

    jElement["Size"][0] = particleSize.x;
    jElement["Size"][1] = particleSize.y;

    jElement["ParticleCount"] = particleCount;

    jElement["Material"] = getMaterialName();

    switch (particleSystemType)
    {
        case ParticleSystemType::Fire: jElement["Type"] = "Fire"; break;
        case ParticleSystemType::Smoke: jElement["Type"] = "Smoke"; break;
    }

    return jElement;
}
