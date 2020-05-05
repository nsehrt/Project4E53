#include "../core/terrain.h"
#include "../util/geogen.h"
#include "../util/serviceprovider.h"
#include "../render/renderresource.h"

Terrain::Terrain(const json& terrainInfo)
{
    auto renderResource = ServiceProvider::getRenderResource();

    cellSpacing = terrainSize / terrainSlices;

    GeometryGenerator geoGen;
    GeometryGenerator::MeshData grid = geoGen.CreateGrid((float)terrainSize, (float)terrainSize,
                                                         terrainSlices, terrainSlices);

    /*create texture descriptor handles*/
    CD3DX12_GPU_DESCRIPTOR_HANDLE baseDescriptor(renderResource->mSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());

    for (UINT i = 0; i < 4; i++)
    {
        blendTexturesHandle[i] = baseDescriptor;
        blendTexturesHandle[i].Offset(renderResource->mTextures[terrainInfo["BlendTextures"][i]]->index, renderResource->mCbvSrvUavDescriptorSize);
        textureStrings[i] = terrainInfo["BlendTextures"][i];
    }

    heightScale = terrainInfo["HeightScale"];

    /*load height map*/
    mHeightMap.resize(terrainSlices * terrainSlices, 0);
    std::vector<unsigned short> input(terrainSlices * terrainSlices, 32767);

    std::stringstream lFile;
    lFile << terrainPath << terrainInfo["HeightMap"].get<std::string>();

    std::ifstream file;
    file.open(lFile.str().c_str(), std::ios_base::binary);

    if (!file.is_open())
    {
        LOG(Severity::Error, "Unable to open height map file " << lFile.str() << "! Proceeding with default values.");
        file.close();
        
    }
    else
    {
        file.read((char*)&input[0], (std::streamsize)input.size() * sizeof(unsigned short));
        file.close();
    }

    terrainHeightMapFile = lFile.str();
    terrainHeightMapFileStem = terrainInfo["HeightMap"].get<std::string>();

    /*load blend map*/
    mBlendMap.resize(terrainSlices * terrainSlices, XMFLOAT4(1.0f, 0.0f, 0.0f, 0.0f));

    lFile.str("");
    lFile << terrainPath << terrainInfo["BlendMap"].get<std::string>();
    terrainBlendMapFile = lFile.str();
    terrainBlendMapFileStem = terrainInfo["BlendMap"].get<std::string>();

    file.open(lFile.str().c_str(), std::ios_base::binary);

    if (file.is_open())
    {
        file.read((char*)&mBlendMap[0], (std::streamsize)mBlendMap.size() * sizeof(DirectX::XMFLOAT4));
    }
    else
    {
        LOG(Severity::Warning, "Unable to open blend map file " << lFile.str() << "! Proceeding with default values.");
    }

    file.close();

    /*copy to actual height map*/
    for (UINT i = 0; i < (UINT)input.size(); i++)
    {
        mHeightMap[i] = (input[i] / 65536.0f) * heightScale - (heightScale / 2.0f);
    }

    mTerrainVertices.resize(grid.Vertices.size());
    std::vector<std::uint32_t> indices(grid.Indices32.size());

    for (size_t i = 0; i < grid.Vertices.size(); i++)
    {
        mTerrainVertices[i].Pos = grid.Vertices[i].Position;
        mTerrainVertices[i].Pos.y = mHeightMap[i];
        mTerrainVertices[i].Normal = grid.Vertices[i].Normal;

        mTerrainVertices[i].TexC = grid.Vertices[i].TexC;
        mTerrainVertices[i].TangentU = grid.Vertices[i].TangentU;

        mTerrainVertices[i].TexBlend = mBlendMap[i];
    }

    indices.insert(indices.end(), std::begin(grid.Indices32), std::end(grid.Indices32));

    UINT vertexByteSize = (UINT)mTerrainVertices.size() * sizeof(TerrainVertex);
    UINT indexByteSize = (UINT)indices.size() * sizeof(std::uint32_t);

    auto geoGrid = std::make_unique<Mesh>();
    geoGrid->name = "TERRAIN";

    geoGrid->VertexByteStride = sizeof(TerrainVertex);
    geoGrid->VertexBufferByteSize = vertexByteSize;
    geoGrid->IndexFormat = DXGI_FORMAT_R32_UINT;
    geoGrid->IndexBufferByteSize = indexByteSize;
    geoGrid->IndexCount = (UINT)indices.size();

    geoGrid->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(renderResource->device,
                                                            renderResource->cmdList,
                                                            mTerrainVertices.data(),
                                                            vertexByteSize,
                                                            geoGrid->VertexBufferUploader);

    geoGrid->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(renderResource->device,
                                                           renderResource->cmdList,
                                                           indices.data(),
                                                           indexByteSize,
                                                           geoGrid->IndexBufferUploader);

    terrainModel = std::make_unique<Model>();
    terrainModel->name = "TERRAIN";
    terrainModel->meshes.push_back(std::move(geoGrid));

    if (!ServiceProvider::getSettings()->miscSettings.EditModeEnabled)
    {
        mTerrainVertices.resize(0);
        mTerrainVertices.shrink_to_fit();
    }
}

float Terrain::getHeight(float x, float z)
{
    // Transform from terrain local space to "cell" space.
    float c = (x + 0.5f * terrainSize) / cellSpacing;
    float d = (z - 0.5f * terrainSize) / -cellSpacing;

    // Get the row and column we are in.
    int row = (int)floorf(d);
    int col = (int)floorf(c);

    // Grab the heights of the cell we are in.
    // A*--*B
    //  | /|
    //  |/ |
    // C*--*D
    float A = mHeightMap[row * terrainSlices + col];
    float B = mHeightMap[row * terrainSlices + col + 1];
    float C = mHeightMap[(row + 1) * terrainSlices + col];
    float D = mHeightMap[(row + 1) * terrainSlices + col + 1];

    // Where we are relative to the cell.
    float s = c - (float)col;
    float t = d - (float)row;

    // If upper triangle ABC.
    if (s + t <= 1.0f)
    {
        float uy = B - A;
        float vy = C - A;
        return A + s * uy + t * vy;
    }
    else // lower triangle DCB.
    {
        float uy = C - D;
        float vy = B - D;
        return D + (1.0f - s) * uy + (1.0f - t) * vy;
    }
}

void Terrain::increaseHeight(float x, float z, float fallStart, float fallEnd, float increase, float resetHeight, bool setHeight)
{
    /*get height of main vertex*/
    float mainVertexHeight = getHeight(x, z) + increase;

    /*iterate over all vertices in the fallEnd * fallEnd cluster*/
    int iterations = static_cast<int>(fallEnd * 2 / cellSpacing) + 1;
    if (iterations % 2 != 0) iterations++;

    float startX = x - (iterations / 2.0f * cellSpacing);
    float startZ = z - (iterations / 2.0f * cellSpacing);

    float xPos, zPos;
    zPos = startZ;

    for (int i = 0; i < iterations; i++)
    {
        xPos = startX;
        zPos += cellSpacing;

        for (int j = 0; j < iterations; j++)
        {
            xPos += cellSpacing;

            /*check if position is inside the selection*/
            float c = (xPos + 0.5f * terrainSize) / cellSpacing;
            float d = (zPos - 0.5f * terrainSize) / -cellSpacing;

            int row = (int)floorf(d);
            int col = (int)floorf(c);

            int currentIndex = row * terrainSlices + col;

            float lengthBetween = sqrtf(powf(xPos - x, 2.0f) +
                                        powf(zPos - z, 2.0f));

            if (lengthBetween > fallEnd) continue;

            /*return if height already higher than height of main selected vertex*/
            if (currentIndex > mHeightMap.size() - 1 || currentIndex < 0) continue;

            if (!setHeight)
            {
                if (mHeightMap[currentIndex] > mainVertexHeight && increase > 0) continue;
                if (mHeightMap[currentIndex] < mainVertexHeight && increase < 0) continue;

                /*normalize distance from center*/
                float normalizedDistance;

                if (lengthBetween < fallStart)
                {
                    normalizedDistance = 1.0f;
                }
                else
                {
                    normalizedDistance = 1.0f - ((lengthBetween - fallStart) / (fallEnd - fallStart));
                }

                /*increase height*/
                mHeightMap[currentIndex] += increase * sin(normalizedDistance * XM_PIDIV2);
            }
            else
            {
                mHeightMap[currentIndex] = resetHeight;
            }

            mHeightMap[currentIndex] = MathHelper::clampH(mHeightMap[currentIndex], -heightScale / 2.0f, heightScale / 2.0f);
            mTerrainVertices[currentIndex].Pos.y = mHeightMap[currentIndex];
        }
    }

    /*copy to gpu*/
    auto terrainVB = ServiceProvider::getRenderResource()->getCurrentFrameResource()->TerrainVB.get();
    terrainVB->copyAll(mTerrainVertices[0]);
    holder = terrainModel->meshes[0]->VertexBufferGPU;
    terrainModel->meshes[0]->VertexBufferGPU = terrainVB->getResource();
}

void Terrain::paint(float x, float z, float fallStart, float fallEnd, float increase, int indexTexture, bool setZero)
{
    /*iterate over all vertices in the fallEnd * fallEnd cluster*/
    int iterations = static_cast<int>(fallEnd * 2 / cellSpacing) + 1;
    if (iterations % 2 != 0) iterations++;

    float startX = x - (iterations / 2.0f * cellSpacing);
    float startZ = z - (iterations / 2.0f * cellSpacing);

    float xPos, zPos;
    zPos = startZ;

    for (int i = 0; i < iterations; i++)
    {
        xPos = startX;
        zPos += cellSpacing;

        for (int j = 0; j < iterations; j++)
        {
            xPos += cellSpacing;

            /*check if position is inside the selection*/
            float c = (xPos + 0.5f * terrainSize) / cellSpacing;
            float d = (zPos - 0.5f * terrainSize) / -cellSpacing;

            int row = (int)floorf(d);
            int col = (int)floorf(c);

            int currentIndex = row * terrainSlices + col;

            float lengthBetween = sqrtf(powf(xPos - x, 2.0f) +
                                        powf(zPos - z, 2.0f));

            if (lengthBetween > fallEnd) continue;

            /*vector out of bounds check*/
            if (currentIndex > mBlendMap.size() - 1 || currentIndex < 0) continue;

            /*normalize distance from center*/
            float normalizedDistance;

            if (lengthBetween < fallStart)
            {
                normalizedDistance = 1.0f;
            }
            else
            {
                normalizedDistance = 1.0f - ((lengthBetween - fallStart) / (fallEnd - fallStart));
            }

            /*increase paint of texture*/
            float normIncrease = increase * normalizedDistance;

            switch (indexTexture)
            {
                case 0:
                    mBlendMap[currentIndex].x = MathHelper::clampH(mBlendMap[currentIndex].x + normIncrease,
                                                                   0.0f, 1.0f);
                    if (setZero) mBlendMap[currentIndex].x = 0.0f;
                    break;
                case 1:
                    mBlendMap[currentIndex].y = MathHelper::clampH(mBlendMap[currentIndex].y + normIncrease,
                                                                   0.0f, 1.0f);
                    if (setZero) mBlendMap[currentIndex].y = 0.0f;
                    break;
                case 2:
                    mBlendMap[currentIndex].z = MathHelper::clampH(mBlendMap[currentIndex].z + normIncrease,
                                                                   0.0f, 1.0f);
                    if (setZero) mBlendMap[currentIndex].z = 0.0f;
                    break;
                case 3:
                    mBlendMap[currentIndex].w = MathHelper::clampH(mBlendMap[currentIndex].w + normIncrease,
                                                                   0.0f, 1.0f);
                    if (setZero) mBlendMap[currentIndex].w = 0.0f;
                    break;
            }

            XMVECTOR t = XMVector4Normalize(XMLoadFloat4(&mBlendMap[currentIndex]));
            XMStoreFloat4(&mBlendMap[currentIndex], t);
            mTerrainVertices[currentIndex].TexBlend = mBlendMap[currentIndex];
        }
    }

    /*copy to gpu*/
    auto terrainVB = ServiceProvider::getRenderResource()->getCurrentFrameResource()->TerrainVB.get();
    terrainVB->copyAll(mTerrainVertices[0]);
    holder = terrainModel->meshes[0]->VertexBufferGPU;
    terrainModel->meshes[0]->VertexBufferGPU = terrainVB->getResource();
}

bool Terrain::save()
{
    return saveBlendMap() && saveHeightMap();
}

bool Terrain::saveBlendMap()
{
    auto fileHandle = std::ofstream(terrainBlendMapFile.c_str(), std::ios::out | std::ios::binary);

    if (!fileHandle.is_open())
    {
        LOG(Severity::Error, "Can not write to " << terrainBlendMapFile << "! " << errno);
        return false;
    }

    fileHandle.write(reinterpret_cast<const char*>(&mBlendMap[0]), sizeof(DirectX::XMFLOAT4) * mBlendMap.size());

    fileHandle.close();

    LOG(Severity::Info, "Successfully wrote blend map to file " << terrainBlendMapFile << ". (" << (sizeof(DirectX::XMFLOAT4) * mBlendMap.size() / 1024.0f) << " kB)");

    return true;
}

bool Terrain::saveHeightMap()
{
    auto fileHandle = std::ofstream(terrainHeightMapFile.c_str(), std::ios::out | std::ios::binary);

    if (!fileHandle.is_open())
    {
        LOG(Severity::Error, "Can not write to " << terrainHeightMapFile << "! " << errno);
        return false;
    }

    unsigned short temp = 0;

    for (UINT i = 0; i < mHeightMap.size(); i++)
    {
        temp = (unsigned short)((mHeightMap[i] + heightScale / 2) / heightScale * 65536);
        fileHandle.write(reinterpret_cast<const char*>(&temp), sizeof(unsigned short));
    }

    fileHandle.close();

    LOG(Severity::Info, "Successfully wrote height map to file " << terrainHeightMapFile << ". (" << (sizeof(unsigned short) * mHeightMap.size() / 1024.0f) << " kB)");

    return true;
}
