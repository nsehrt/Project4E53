#include "modelloader.h"

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

    //m->meshes.reserve((size_t)numMeshes);

    mRet.model = std::make_unique<Model>();
    mRet.model->name = fileName.path().filename().string();

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

        char* dmap = new char[(static_cast<long>(slen))+1];
        file.read(dmap, slen);
        dmap[slen] = '\0';

        slen = 0;
        file.read((char*)(&slen), sizeof(short));

        char* nmap = new char[ (static_cast<long>(slen)) + 1];
        file.read(nmap, slen);
        nmap[slen] = '\0';

        slen = 0;
        file.read((char*)(&slen), sizeof(short));

        char* bmap = new char[ (static_cast<long>(slen)) + 1];
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
        m->DrawArgs["all"].StartIndexLocation = 0;
        m->DrawArgs["all"].BaseVertexLocation = 0;
        m->DrawArgs["all"].IndexCount = (UINT)indices.size();
        
        mRet.model->meshes[std::to_string(i)] = std::move(m);
    }

    /*finalize collision*/

    //XMStoreFloat3(&m->collisionBox.Center, 0.5f * (vMin + vMax));
    //XMStoreFloat3(&m->collisionBox.Extents, 0.5f * (vMax - vMin));
    
    return mRet;
}
