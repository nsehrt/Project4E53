#include "skinnedmodelloader.h"
#include "geogen.h"

using namespace DirectX;

std::unique_ptr<Model> SkinnedModelLoader::loadS3D(const std::filesystem::directory_entry& fileName)
{
    /*open file*/
    std::streampos fileSize;
    std::ifstream file(fileName.path(), std::ios::binary);

    /*get file size*/
    file.seekg(0, std::ios::end);
    fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    /*check header*/
    bool header = true;
    char headerBuffer[4] = { 's','3','d','f' };

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
        return nullptr;
    }

    /*number of bones*/
    char numBones = 0;
    file.read((char*)(&numBones), sizeof(char));


    /*bone information*/
    std::vector<int> boneID(numBones);
    std::vector<std::string> boneName(numBones);
    std::vector<DirectX::XMFLOAT4X4> boneOffset(numBones);

    for (char i = 0; i < numBones; i++)
    {
        /*id*/
        file.read((char*)(&boneID[i]), sizeof(char));

        /*name length*/
        short slen = 0;
        file.read((char*)(&slen), sizeof(short));

        /*name*/
        char* bname = new char[(INT_PTR)slen + 1];
        file.read(bname, slen);
        bname[slen] = '\0';

        boneName[i] = std::string(bname);
        delete[] bname;

        /*matrix*/
        file.read((char*)&boneOffset[i]._11, sizeof(float));
        file.read((char*)&boneOffset[i]._12, sizeof(float));
        file.read((char*)&boneOffset[i]._13, sizeof(float));
        file.read((char*)&boneOffset[i]._14, sizeof(float));

        file.read((char*)&boneOffset[i]._21, sizeof(float));
        file.read((char*)&boneOffset[i]._22, sizeof(float));
        file.read((char*)&boneOffset[i]._23, sizeof(float));
        file.read((char*)&boneOffset[i]._24, sizeof(float));

        file.read((char*)&boneOffset[i]._31, sizeof(float));
        file.read((char*)&boneOffset[i]._32, sizeof(float));
        file.read((char*)&boneOffset[i]._33, sizeof(float));
        file.read((char*)&boneOffset[i]._34, sizeof(float));

        file.read((char*)&boneOffset[i]._41, sizeof(float));
        file.read((char*)&boneOffset[i]._42, sizeof(float));
        file.read((char*)&boneOffset[i]._43, sizeof(float));
        file.read((char*)&boneOffset[i]._44, sizeof(float));
    }

    /*bone hierarchy*/
    std::vector<int> boneHierarchy(numBones);

    for (char i = 0; i < numBones; i++)
    {
        int index;

        file.read((char*)(&index), sizeof(int));
        file.read((char*)(&boneHierarchy[index]), sizeof(int));
    }

    /*number of meshes*/
    char numMeshes = 0;
    file.read(&numMeshes, sizeof(numMeshes));

    std::unique_ptr<Model> mRet = std::make_unique<Model>();

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

        char* matStr = new char[(INT_PTR)slen + 1];
        file.read(matStr, slen);
        matStr[slen] = '\0';

        m->materialName = matStr;

        delete[] matStr;

        /*read number of vertices*/
        int vertCount = 0;
        file.read((char*)(&vertCount), sizeof(vertCount));

        std::vector<Vertex> vertices(vertCount);

        UINT temporary;

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

            /*bone indices & weights*/
            file.read((char*)&temporary, sizeof(UINT));
            file.read((char*)&temporary, sizeof(float));

            file.read((char*)&temporary, sizeof(UINT));
            file.read((char*)&temporary, sizeof(float));

            file.read((char*)&temporary, sizeof(UINT));
            file.read((char*)&temporary, sizeof(float));

            file.read((char*)&temporary, sizeof(UINT));
            file.read((char*)&temporary, sizeof(float));


            /*collision box related*/
            XMVECTOR P = XMLoadFloat3(&vertices[j].Pos);

            vMin = XMVectorMin(vMin, P);
            vMax = XMVectorMax(vMax, P);
        }

        /*number of indices*/
        int vInd = 0;
        file.read((char*)(&vInd), sizeof(vInd));

        std::vector<unsigned short> indices(vInd);

        int temp = 0;

        for (int j = 0; j < vInd; j++)
        {
            file.read((char*)(&temp), sizeof(int));
            indices.push_back((unsigned short)temp);
        }

        const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);
        const UINT ibByteSize = (UINT)indices.size() * sizeof(unsigned short);

        m->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(device,
                                                          cmdList, vertices.data(), vbByteSize, m->VertexBufferUploader);

        m->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(device,
                                                         cmdList, indices.data(), ibByteSize, m->IndexBufferUploader);

        m->VertexByteStride = sizeof(Vertex);
        m->VertexBufferByteSize = vbByteSize;
        m->IndexFormat = DXGI_FORMAT_R16_UINT;
        m->IndexBufferByteSize = ibByteSize;
        m->IndexCount = (UINT)indices.size();

        mRet->group = fileName.path().parent_path().filename().string();
        mRet->meshes.push_back(std::move(m));
        indices.clear();
    }

    /*create AABB*/
    XMStoreFloat3(&mRet->boundingBox.Center, 0.5f * (vMin + vMax));
    XMStoreFloat3(&mRet->boundingBox.Extents, 0.5f * (vMax - vMin));

    XMStoreFloat3(&mRet->frustumBoundingBox.Center, 0.5f * (vMin + vMax));
    XMStoreFloat3(&mRet->frustumBoundingBox.Extents, 0.5f * (vMax - vMin));

    GeometryGenerator geoGen;
    GeometryGenerator::MeshData boxMesh = geoGen.CreateBox(mRet->boundingBox.Extents.x * 2.f,
                                                           mRet->boundingBox.Extents.y * 2.f,
                                                           mRet->boundingBox.Extents.z * 2.f,
                                                           0);

    std::vector<Vertex> vertices(boxMesh.Vertices.size());
    std::vector<std::uint16_t> indices(boxMesh.Indices32.size());

    for (size_t i = 0; i < boxMesh.Vertices.size(); i++)
    {
        XMStoreFloat3(&vertices[i].Pos, XMVectorAdd(XMLoadFloat3(&boxMesh.Vertices[i].Position), XMLoadFloat3(&mRet->boundingBox.Center)));
        vertices[i].Normal = boxMesh.Vertices[i].Normal;
        vertices[i].TexC = boxMesh.Vertices[i].TexC;
        vertices[i].TangentU = boxMesh.Vertices[i].TangentU;
    }

    indices.insert(indices.end(), std::begin(boxMesh.GetIndices16()), std::end(boxMesh.GetIndices16()));

    UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);
    UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

    std::unique_ptr<Mesh> hitbox = std::make_unique<Mesh>();

    hitbox->IndexFormat = DXGI_FORMAT_R16_UINT;
    hitbox->VertexByteStride = sizeof(Vertex);
    hitbox->VertexBufferByteSize = vbByteSize;
    hitbox->IndexBufferByteSize = ibByteSize;
    hitbox->IndexCount = (UINT)indices.size();

    hitbox->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(device,
                                                           cmdList, vertices.data(), vbByteSize, hitbox->VertexBufferUploader);

    hitbox->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(device,
                                                          cmdList, indices.data(), ibByteSize, hitbox->IndexBufferUploader);

    mRet->boundingBoxMesh = std::move(hitbox);
    mRet->name = fileName.path().stem().string();

    return mRet;
}
