#include "modelloader.h"
#include "geogen.h"

using namespace DirectX;

ModelReturn ModelLoader::loadB3D(const std::filesystem::directory_entry& fileName)
{
    ModelReturn mRet;

    /*open file*/
    std::streampos fileSize;
    std::ifstream file(fileName.path(), std::ios::binary);

    /*get file size*/
    file.seekg(0, std::ios::end);
    fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    /*check header*/
    bool header = true;
    char headerBuffer[4] = { 'b','3','d','f' };

    for (int i = 0; i < 4; i++)
    {
        char temp;
        file.read(&temp, sizeof(temp));

        if (temp != headerBuffer[i])
        {
            header = false;
            break;
        }
    }

    if (header == false)
    {
        /*header incorrect*/
        mRet.errorCode = 1;
        return mRet;
    }

    /*number of meshes*/
    char numMeshes = 0;
    file.read(&numMeshes, sizeof(numMeshes));

    mRet.model = std::make_unique<Model>();

    XMFLOAT3 cMin(+FLT_MAX, +FLT_MAX, +FLT_MAX);
    XMFLOAT3 cMax(-FLT_MAX, -FLT_MAX, -FLT_MAX);

    XMVECTOR vMin = XMLoadFloat3(&cMin);
    XMVECTOR vMax = XMLoadFloat3(&cMax);

    for (char i = 0; i < numMeshes; i++)
    {
        std::unique_ptr<Mesh> m = std::make_unique<Mesh>();

        /*read map strings*/

        short slen = 0;
        file.read((char*)(&slen), sizeof(short));

        char* dmap = new char[(long long)slen +1];
        file.read(dmap, slen);
        dmap[slen] = '\0';

        slen = 0;
        file.read((char*)(&slen), sizeof(short));

        char* nmap = new char[(long long)slen + 1];
        file.read(nmap, slen);
        nmap[slen] = '\0';

        slen = 0;
        file.read((char*)(&slen), sizeof(short));

        char* bmap = new char[(long long)slen + 1];
        file.read(bmap, slen);
        bmap[slen] = '\0';

        m->dTexture = dmap;
        m->dNormal = nmap;
        m->dBump = bmap;

        delete[] dmap; delete[] nmap; delete[] bmap;

        /*read number of vertices*/
        int vertCount = 0;
        file.read((char*)(&vertCount), sizeof(vertCount));

        std::vector<Vertex> vertices(vertCount);

        /*vertex properties*/
        for (int j = 0; j < vertCount; j++)
        {
            file.read((char*)(&vertices[j].Pos.x), sizeof(float));
            file.read((char*)(&vertices[j].Pos.y), sizeof(float));
            file.read((char*)(&vertices[j].Pos.z), sizeof(float));

            file.read((char*)(&vertices[j].TexC.x), sizeof(float));
            file.read((char*)(&vertices[j].TexC.y), sizeof(float));

            file.read((char*)(&vertices[j].Normal.x), sizeof(float));
            file.read((char*)(&vertices[j].Normal.y), sizeof(float));
            file.read((char*)(&vertices[j].Normal.z), sizeof(float));


            file.read((char*)(&vertices[j].TangentU.x), sizeof(float));
            file.read((char*)(&vertices[j].TangentU.y), sizeof(float));
            file.read((char*)(&vertices[j].TangentU.z), sizeof(float));

            /*collision box related*/
            XMVECTOR P = XMLoadFloat3(&vertices[j].Pos);

            vMin = XMVectorMin(vMin, P);
            vMax = XMVectorMax(vMax, P);


        }

        /*number of indices*/
        std::vector<std::uint16_t> indices;

        int vInd = 0;
        file.read((char*)(&vInd), sizeof(vInd));
        indices.resize(vInd);

        for (int j = 0; j < vInd; j++)
        {
            file.read((char*)(&indices[j]), sizeof(int));
        }

        const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);
        const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);



        ThrowIfFailed(D3DCreateBlob(vbByteSize, &m->VertexBufferCPU));
        CopyMemory(m->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

        ThrowIfFailed(D3DCreateBlob(ibByteSize, &m->IndexBufferCPU));
        CopyMemory(m->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

        m->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(device,
                                                            cmdList, vertices.data(), vbByteSize, m->VertexBufferUploader);

        m->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(device,
                                                           cmdList, indices.data(), ibByteSize, m->IndexBufferUploader);

        m->VertexByteStride = sizeof(Vertex);
        m->VertexBufferByteSize = vbByteSize;
        m->IndexFormat = DXGI_FORMAT_R16_UINT;
        m->IndexBufferByteSize = ibByteSize;
        m->IndexCount = (UINT)indices.size();
        
        mRet.model->meshes[std::to_string(i)] = std::move(m);
    }

    /*create AABB*/
    XMStoreFloat3(&mRet.model->boundingBox.Center, 0.5f * (vMin + vMax));
    XMStoreFloat3(&mRet.model->boundingBox.Extents, 0.5f * (vMax - vMin));

    GeometryGenerator geoGen;
    GeometryGenerator::MeshData boxMesh = geoGen.CreateBox(mRet.model->boundingBox.Extents.x * 2.f,
                                                           mRet.model->boundingBox.Extents.y * 2.f,
                                                           mRet.model->boundingBox.Extents.z * 2.f,
                                                           0);

    std::vector<Vertex> vertices(boxMesh.Vertices.size());
    std::vector<std::uint16_t> indices(boxMesh.Indices32.size());


    for (size_t i = 0; i < boxMesh.Vertices.size(); i++)
    {
        XMStoreFloat3(&vertices[i].Pos, XMVectorAdd(XMLoadFloat3(&boxMesh.Vertices[i].Position), XMLoadFloat3(&mRet.model->boundingBox.Center)));
        vertices[i].Normal = boxMesh.Vertices[i].Normal;
        vertices[i].TexC = boxMesh.Vertices[i].TexC;
        vertices[i].TangentU = boxMesh.Vertices[i].TangentU;
    }

    indices.insert(indices.end(), std::begin(boxMesh.GetIndices16()), std::end(boxMesh.GetIndices16()));

    UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);
    UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);



    std::unique_ptr<Mesh> hitbox = std::make_unique<Mesh>();

    hitbox->name = "hitbox";
    hitbox->dTexture = "default";
    hitbox->dNormal = "defaultNormal";
    hitbox->IndexFormat = DXGI_FORMAT_R16_UINT;
    hitbox->VertexByteStride = sizeof(Vertex);
    hitbox->VertexBufferByteSize = vbByteSize;
    hitbox->IndexBufferByteSize = ibByteSize;
    hitbox->IndexCount = (UINT)indices.size();

    ThrowIfFailed(D3DCreateBlob(vbByteSize, &hitbox->VertexBufferCPU));
    CopyMemory(hitbox->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

    ThrowIfFailed(D3DCreateBlob(ibByteSize, &hitbox->IndexBufferCPU));
    CopyMemory(hitbox->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

    hitbox->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(device,
                                                        cmdList, vertices.data(), vbByteSize, hitbox->VertexBufferUploader);

    hitbox->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(device,
                                                       cmdList, indices.data(), ibByteSize, hitbox->IndexBufferUploader);


    mRet.model->hitboxMesh = std::move(hitbox);

    return mRet;
}
