#include "skinnedmodelloader.h"
#include "geogen.h"

using namespace DirectX;

std::unique_ptr<SkinnedModel> SkinnedModelLoader::loadS3D(const std::filesystem::directory_entry& fileName)
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
    std::vector<std::pair<std::string, DirectX::XMFLOAT4X4>> boneOffset(numBones);

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
        float tFloat[16];
        file.read((char*)&tFloat[0], sizeof(float) * 16);
        boneOffset[i].first = boneName[i];

        XMFLOAT4X4 tMat = XMFLOAT4X4(tFloat);
        XMStoreFloat4x4(&boneOffset[i].second, MathHelper::transposeFromXMFloat(tMat));

    }

    /*bone hierarchy*/
    std::vector<int> boneHierarchy(numBones);

    for (char i = 0; i < numBones; i++)
    {
        int index;

        file.read((char*)(&index), sizeof(int));
        file.read((char*)(&boneHierarchy[index]), sizeof(int));
    }


    /*node tree*/
    std::function<void(Node*, Node*)> loadTree = [&](Node* node, Node* parent)
    {
        /*read data*/
        short slen = 0;
        file.read((char*)(&slen), sizeof(short));

        char* nameStr = new char[(INT_PTR)slen + 1];
        file.read(nameStr, slen);
        nameStr[slen] = '\0';

        float mTemp[16];
        file.read((char*)&mTemp[0], sizeof(float) * 16);

        int numChildren = 0;
        file.read((char*)(&numChildren), sizeof(int));


        /*fill node*/
        node->name = nameStr;
        XMFLOAT4X4 tMat = XMFLOAT4X4(mTemp);
        XMStoreFloat4x4(&node->transform, MathHelper::transposeFromXMFloat(tMat));
        node->parent = parent;

        delete[] nameStr;

        XMStoreFloat4x4(&node->boneOffset, XMMatrixIdentity());
        for (int i = 0; i < (int)boneOffset.size(); i++)
        {
            if (node->name == boneOffset[i].first)
            {
                node->isBone = true;
                node->boneOffset = boneOffset[i].second;
                node->boneIndex = i;
            }
        }

        for (int i = 0; i < numChildren; i++)
        {
            node->children.push_back(new Node());
            loadTree(node->children.back(), node);
        }


    };


   

    /*put bone data into model*/
    std::unique_ptr<SkinnedModel> mRet = std::make_unique<SkinnedModel>();

    mRet->boneCount = numBones;
    mRet->boneHierarchy = boneHierarchy;
    loadTree(mRet->nodeTree.root, nullptr);
    mRet->nodeTree.boneRoot = mRet->nodeTree.findNodeByBoneIndex(0);

    LOG(Severity::Debug, "\n" << mRet->nodeTree.toString() << std::endl);

    auto det = XMMatrixDeterminant(XMLoadFloat4x4(&mRet->nodeTree.root->transform));
    XMStoreFloat4x4(&mRet->globalInverse,XMMatrixInverse(&det, XMLoadFloat4x4(&mRet->nodeTree.root->transform)));

    /*number of meshes*/
    char numMeshes = 0;
    file.read(&numMeshes, sizeof(numMeshes));

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

        std::vector<SkinnedVertex> vertices(vertCount);

        int tempInt;
        float tempFloat;

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
            file.read((char*)&tempInt, sizeof(UINT));
            file.read((char*)&tempFloat, sizeof(float));

            vertices[j].BoneIndices[0] = (BYTE)tempInt;
            vertices[j].BoneWeights.x = tempFloat;

            file.read((char*)&tempInt, sizeof(UINT));
            file.read((char*)&tempFloat, sizeof(float));

            vertices[j].BoneIndices[1] = (BYTE)tempInt;
            vertices[j].BoneWeights.y = tempFloat;

            file.read((char*)&tempInt, sizeof(UINT));
            file.read((char*)&tempFloat, sizeof(float));

            vertices[j].BoneIndices[2] = (BYTE)tempInt;
            vertices[j].BoneWeights.z = tempFloat;

            file.read((char*)&tempInt, sizeof(UINT));
            file.read((char*)&tempFloat, sizeof(float));

            vertices[j].BoneIndices[3] = (BYTE)tempInt;

            /*collision box related*/
            XMVECTOR P = XMLoadFloat3(&vertices[j].Pos);

            vMin = XMVectorMin(vMin, P);
            vMax = XMVectorMax(vMax, P);
        }

        /*number of indices*/
        int vInd = 0;
        file.read((char*)(&vInd), sizeof(vInd));

        std::vector<std::uint16_t> indices(vInd);

        int tri1, tri2, tri3;

        for (int j = 0; j < vInd / 3; j++)
        {
            file.read((char*)(&tri1), sizeof(int));
            file.read((char*)(&tri2), sizeof(int));
            file.read((char*)(&tri3), sizeof(int));
            indices[(INT_PTR)j * 3] = (unsigned short)tri1;
            indices[(INT_PTR)j * 3 + 1] = (unsigned short)tri2;
            indices[(INT_PTR)j * 3 + 2] = (unsigned short)tri3;
        }

        const UINT vbByteSize = (UINT)vertices.size() * sizeof(SkinnedVertex);
        const UINT ibByteSize = (UINT)indices.size() * sizeof(unsigned short);

        m->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(device,
                                                          cmdList, vertices.data(), vbByteSize, m->VertexBufferUploader);

        m->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(device,
                                                         cmdList, indices.data(), ibByteSize, m->IndexBufferUploader);

        m->VertexByteStride = sizeof(SkinnedVertex);
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
