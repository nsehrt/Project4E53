#include "grass.h"

void Grass::create(const json& grassJson, Terrain* terrain)
{
    /*read properties from json*/
    position.x = grassJson["Position"][0];
    position.y = grassJson["Position"][1];
    position.z = grassJson["Position"][2];

    size.x = grassJson["Size"][0];
    size.y = grassJson["Size"][1];

    quadSize.x = grassJson["QuadSize"][0];
    quadSize.y = grassJson["QuadSize"][1];

    materialName = grassJson["Material"];

    density.x = grassJson["Density"][0];
    density.y = grassJson["Density"][1];

    sizeVariation = grassJson["SizeVariation"];

    /*create mesh from parameters*/

    GeometryGenerator geoGen;
    GeometryGenerator::MeshData grassTest = geoGen.CreateGrid(size.x, size.y, density.x, density.y);

    std::vector<BillBoardVertex> vertices(grassTest.Vertices.size());
    std::vector<std::uint16_t> indices(grassTest.Vertices.size());

    for (size_t i = 0; i < grassTest.Vertices.size(); i++)
    {
        vertices[i].Pos = grassTest.Vertices[i].Position;

        XMFLOAT3 worldPos = vertices[i].Pos;

        worldPos.x += position.x;
        worldPos.z += position.z;

        vertices[i].Pos.y = terrain->getHeight(worldPos.x, worldPos.z) + (quadSize.y * 0.5f);

        float variation = MathHelper::randF(0, 2 * sizeVariation) - sizeVariation;

        vertices[i].Size = quadSize;
        vertices[i].Size.x += variation;
        vertices[i].Size.y += variation;

    }

    for (int i = 0; i < indices.size(); i++)
    {
        indices[i] = i;
    }

    UINT vbByteSize = (UINT)vertices.size() * sizeof(BillBoardVertex);
    UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

    auto geo = std::make_unique<Mesh>();

    ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU));
    CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

    ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
    CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

    geo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(renderResource->device,
                                                        renderResource->cmdList, vertices.data(), vbByteSize, geo->VertexBufferUploader);

    geo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(renderResource->device,
                                                       renderResource->cmdList, indices.data(), ibByteSize, geo->IndexBufferUploader);

    geo->VertexByteStride = sizeof(BillBoardVertex);
    geo->VertexBufferByteSize = vbByteSize;
    geo->IndexFormat = DXGI_FORMAT_R16_UINT;
    geo->IndexBufferByteSize = ibByteSize;
    geo->IndexCount = (UINT)indices.size();

    grassPatchModel = std::make_unique<Model>();

    grassPatchModel->name = "GRASS_PATCH";
    grassPatchModel->group = "grass";
    grassPatchModel->meshes.push_back(std::move(geo));

}

json Grass::toJson()
{
    json jElement;

    jElement["Position"][0] = getPosition().x;
    jElement["Position"][1] = getPosition().y;
    jElement["Position"][2] = getPosition().z;

    jElement["Size"][0] = getSize().x;
    jElement["Size"][1] = getSize().y;

    jElement["Material"] = getMaterialName();

    jElement["QuadSize"][0] = quadSize.x;
    jElement["QuadSize"][1] = quadSize.y;

    jElement["Density"][0] = getDensity().x;
    jElement["Density"][1] = getDensity().y;

    jElement["SizeVariation"] = sizeVariation;

    return jElement;
}
