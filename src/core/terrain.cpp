#include "../core/terrain.h"
#include "../util/geogen.h"
#include "../util/serviceprovider.h"
#include "../render/renderresource.h"

Terrain::Terrain(const std::string& heightMap)
{

    GeometryGenerator geoGen;

    GeometryGenerator::MeshData grid = geoGen.CreateGrid(TERRAIN_SIZE, TERRAIN_SIZE,
                                                              TERRAIN_SLICES, TERRAIN_SLICES);

    /*load height map*/
    mHeightMap.resize(HEIGHTMAP_SIZE * HEIGHTMAP_SIZE,0);
    std::vector<unsigned char> input(HEIGHTMAP_SIZE * HEIGHTMAP_SIZE);

    std::stringstream lFile;
    lFile << TERRAIN_PATH << heightMap;

    std::ifstream file;
    file.open(lFile.str().c_str(), std::ios_base::binary);

    if (!file.is_open())
    {
        LOG(Severity::Error, "Unable to open height map file "<< heightMap << "!");
        return;
    }

    file.read((char*)&input[0], (std::streamsize)input.size());
    file.close();

    /*copy to actual height map*/
    for (UINT i = 0; i < (UINT)input.size(); i++)
    {
        mHeightMap[i] = (input[i] / 255.0f) * HEIGHTMAP_SCALE;
    }


    std::vector<Vertex> vertices(grid.Vertices.size());
    std::vector<std::uint16_t> indices(grid.Indices32.size());

    for (size_t i = 0; i < grid.Vertices.size(); i++)
    {
        auto& p = grid.Vertices[i].Position;
        vertices[i].Pos = p;
        vertices[i].Pos.y = calculateHeight(p.x, p.z) * 0.2f;
        //int index = (int)(i / float(grid.Vertices.size()) * (HEIGHTMAP_SIZE * HEIGHTMAP_SIZE));
        //vertices[i].Pos.y = mHeightMap[index];

        vertices[i].Normal = grid.Vertices[i].Normal;
        vertices[i].Normal = calculateNormal(p.x, p.z);


        vertices[i].TexC = grid.Vertices[i].TexC;
        vertices[i].TangentU = grid.Vertices[i].TangentU;

    }

    /*TODO HEIGHT*/


    indices.insert(indices.end(), std::begin(grid.GetIndices16()), std::end(grid.GetIndices16()));

    UINT vertexByteSize = (UINT)vertices.size() * sizeof(Vertex);
    UINT indexByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

    auto geoGrid = std::make_unique<Mesh>();
    geoGrid->name = "TERRAIN";

    ThrowIfFailed(D3DCreateBlob(vertexByteSize, &geoGrid->VertexBufferCPU));
    CopyMemory(geoGrid->VertexBufferCPU->GetBufferPointer(), vertices.data(), vertexByteSize);

    ThrowIfFailed(D3DCreateBlob(indexByteSize, &geoGrid->IndexBufferCPU));
    CopyMemory(geoGrid->IndexBufferCPU->GetBufferPointer(), indices.data(), indexByteSize);

    geoGrid->VertexByteStride = sizeof(Vertex);
    geoGrid->VertexBufferByteSize = vertexByteSize;
    geoGrid->IndexFormat = DXGI_FORMAT_R16_UINT;
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

void Terrain::draw()
{

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
