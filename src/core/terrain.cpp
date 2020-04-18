#include "../core/terrain.h"
#include "../util/geogen.h"
#include "../util/serviceprovider.h"
#include "../render/renderresource.h"

Terrain::Terrain(const std::string& heightMap)
{

    GeometryGenerator geoGen;

    GeometryGenerator::MeshData grid = geoGen.CreateGrid((float)terrainSize, (float)terrainSize,
                                                              terrainSlices, terrainSlices);

    /*load height map*/
    mHeightMap.resize(terrainSlices * terrainSlices,0);
    std::vector<unsigned short> input(terrainSlices * terrainSlices);

    std::stringstream lFile;
    lFile << terrainPath << heightMap;

    std::ifstream file;
    file.open(lFile.str().c_str(), std::ios_base::binary);

    if (!file.is_open())
    {
        LOG(Severity::Error, "Unable to open height map file "<< heightMap << "!");
        return;
    }

    terrainFile = lFile.str();

    file.read((char*)&input[0], (std::streamsize)input.size() * 2);
    file.close();

    /*copy to actual height map*/
    for (UINT i = 0; i < (UINT)input.size(); i++)
    {
        mHeightMap[i] = (input[i] / 65536.0f) * heightScale - (heightScale/2.0f);
    }

    //smoothHeightMap();

    std::vector<Vertex> vertices(grid.Vertices.size());
    std::vector<std::uint32_t> indices(grid.Indices32.size());

    for (size_t i = 0; i < grid.Vertices.size(); i++)
    {
        vertices[i].Pos = grid.Vertices[i].Position;
        vertices[i].Pos.y = mHeightMap[i];
        vertices[i].Normal = grid.Vertices[i].Normal;

        vertices[i].TexC = grid.Vertices[i].TexC;
        vertices[i].TangentU = grid.Vertices[i].TangentU;

    }


    indices.insert(indices.end(), std::begin(grid.Indices32), std::end(grid.Indices32));

    UINT vertexByteSize = (UINT)vertices.size() * sizeof(Vertex);
    UINT indexByteSize = (UINT)indices.size() * sizeof(std::uint32_t);

    auto geoGrid = std::make_unique<Mesh>();
    geoGrid->name = "TERRAIN";

    ThrowIfFailed(D3DCreateBlob(vertexByteSize, &geoGrid->VertexBufferCPU));
    CopyMemory(geoGrid->VertexBufferCPU->GetBufferPointer(), vertices.data(), vertexByteSize);

    ThrowIfFailed(D3DCreateBlob(indexByteSize, &geoGrid->IndexBufferCPU));
    CopyMemory(geoGrid->IndexBufferCPU->GetBufferPointer(), indices.data(), indexByteSize);

    geoGrid->VertexByteStride = sizeof(Vertex);
    geoGrid->VertexBufferByteSize = vertexByteSize;
    geoGrid->IndexFormat = DXGI_FORMAT_R32_UINT;
    geoGrid->IndexBufferByteSize = indexByteSize;
    geoGrid->IndexCount = (UINT)indices.size();

    auto renderResource = ServiceProvider::getRenderResource();

    geoGrid->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(renderResource->device,
                                                                renderResource->cmdList,
                                                                vertices.data(),
                                                                vertexByteSize,
                                                                geoGrid->VertexBufferUploader);

    geoGrid->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(renderResource->device,
                                                               renderResource->cmdList,
                                                               indices.data(),
                                                               indexByteSize,
                                                               geoGrid->IndexBufferUploader);

    terrainModel = std::make_unique<Model>();
    terrainModel->name = "TERRAIN";
    terrainModel->meshes["TERRAIN"] = std::move(geoGrid);
}

void Terrain::save()
{

    auto fileHandle = std::fstream(terrainFile.c_str(), std::ios::out | std::ios::binary);

    if (!fileHandle.is_open())
    {
        LOG(Severity::Error, "Can not write to " << terrainFile << "!");
        return;
    }

    unsigned short temp = 0;

    for (UINT i = 0; i < mHeightMap.size(); i++)
    {
        temp = (mHeightMap[i] + heightScale / 2) / heightScale * 65536;
        fileHandle.write(reinterpret_cast<const char*>(&temp), sizeof(unsigned short));
    }
 
    fileHandle.close();

    LOG(Severity::Info, "Successfully wrote height map to file " << terrainFile << ". (" << (sizeof(unsigned short) * mHeightMap.size() / 1024.0f) << " kB)");
}

float Terrain::calculateHeight(float x, float z) const
{
    return 0.3f * (z * sinf(0.1f * x) + x * cosf(0.1f * z));
}

DirectX::XMFLOAT3 Terrain::calculateNormal(float x, float z) const
{
    XMFLOAT3 n(
        -0.03f * z * cosf(0.1f * x) - 0.3f * cosf(0.1f * z),
        1.0f,
        -0.3f * sinf(0.1f * x) + 0.03f * x * sinf(0.1f * z));

    XMVECTOR unitNormal = XMVector3Normalize(XMLoadFloat3(&n));
    XMStoreFloat3(&n, unitNormal);

    return n;
}

float Terrain::getWidth() const
{
    return 0.0f;
}

float Terrain::getDepth() const
{
    return 0.0f;
}

float Terrain::getHeight(float x, float y) const
{
    return 0.0f;
}
