#include "renderresource.h"
#include "../util/serviceprovider.h"

bool RenderResource::init(ID3D12Device* _device, ID3D12GraphicsCommandList* _cmdList, const std::filesystem::path& _texturePath, const std::filesystem::path& _modelPath)
{
    device = _device;
    cmdList = _cmdList;

    int textureCounter = 0;
    int modelCounter = 0;

    mHeapDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    buildRootSignature();
    buildShaders();
    buildInputLayouts();


    /*load all textures*/

    const UINT texTypes = 2;
    std::stringstream tstr[texTypes];
    tstr[0] << _texturePath.string() << "/tex2d";
    tstr[1] << _texturePath.string() << "/texcube";

    int texCounter[texTypes] = { 0 };

    int tC = 0;

    for (auto const& s : tstr)
    {
        for (const auto& entry : std::filesystem::recursive_directory_iterator(std::filesystem::path(s.str())))
        {
            if (loadTexture(entry, static_cast<TextureType>(tC)))
            {
                textureCounter++;
                texCounter[tC]++;
            }
            else
            {
                std::stringstream str;
                str << "Failed to load texture " << entry.path().u8string() << "!";
                ServiceProvider::getVSLogger()->print<Severity::Warning>(str.str().c_str());
            }
        }
        tC++;
    }


    buildDescriptorHeap();

    /*list loaded textures*/
    std::stringstream str;
    str << "Successfully loaded " << textureCounter << " textures. (";
    for (UINT i = 0; i < texTypes; i++)
    {
        str << texCounter[i];
        if (i != texTypes - 1)
            str << ", ";
    }
    str << ")";

    ServiceProvider::getVSLogger()->print<Severity::Info>(str.str().c_str());


    /*load all models*/
    for (const auto& entry : std::filesystem::recursive_directory_iterator(_modelPath))
    {
        if (loadModel(entry.path().u8string()))
        {
            modelCounter++;
        }
        else
        {
            std::stringstream str;
            str << "Failed to load model " << entry.path().u8string() << "!";
            ServiceProvider::getVSLogger()->print<Severity::Warning>(str.str().c_str());
        }
    }

    str.str("");
    str << "Successfully loaded " << modelCounter << " models.";
    ServiceProvider::getVSLogger()->print<Severity::Info>(str.str().c_str());


    /*also generate some default shapes*/

    generateDefaultShapes();
    buildPSOs();

    if (!buildMaterials())
    {
        return false;
    }

    buildRenderItems();

    return true;
}

bool RenderResource::loadTexture(const std::filesystem::directory_entry& file, TextureType type)
{
    auto texMap = std::make_unique<Texture>();
    texMap->Name = file.path().filename().u8string();
    texMap->Filename = AnsiToWString(file.path().u8string());
    texMap->Type = type;

    HRESULT hr = DirectX::CreateDDSTextureFromFile12(device, cmdList, texMap->Filename.c_str(), texMap->Resource, texMap->UploadHeap);
    mTextures[texMap->Name] = std::move(texMap);

    return hr == S_OK;
}

bool RenderResource::loadModel(const std::string& file)
{
    return false;
}

bool RenderResource::buildRootSignature()
{
    /*5 root parameter*/
    CD3DX12_ROOT_PARAMETER rootParameter[5];

    /*1 texture in register 0*/
    CD3DX12_DESCRIPTOR_RANGE textureTableReg0;
    textureTableReg0.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0);

    /*10 textures in register 1*/
    CD3DX12_DESCRIPTOR_RANGE textureTableReg1;
    textureTableReg1.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 10, 1, 0);

    /*constant buffer views in register b0 and b1*/
    rootParameter[0].InitAsConstantBufferView(0);
    rootParameter[1].InitAsConstantBufferView(1);

    /*material data in reg 0 space 1*/
    rootParameter[2].InitAsShaderResourceView(0, 1);

    /*t0,t1 visible for pixel shader*/
    rootParameter[3].InitAsDescriptorTable(1, &textureTableReg0, D3D12_SHADER_VISIBILITY_PIXEL);
    rootParameter[4].InitAsDescriptorTable(1, &textureTableReg1, D3D12_SHADER_VISIBILITY_PIXEL);

    /*get the static samplers and bind them to root signature description*/
    auto staticSamplers = GetStaticSamplers();

    CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDescription(5, rootParameter, (UINT)staticSamplers.size(),
                                                         staticSamplers.data(), D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    /*create root signature using the description*/
    ComPtr<ID3DBlob> serializedRootSignature = nullptr;
    ComPtr<ID3DBlob> errorBlob = nullptr;

    HRESULT hr = D3D12SerializeRootSignature(&rootSignatureDescription, D3D_ROOT_SIGNATURE_VERSION_1,
                                             serializedRootSignature.GetAddressOf(), errorBlob.GetAddressOf());

    if (errorBlob != nullptr)
    {
        std::stringstream str;
        str << "Error serializing root signature: " << (char*)errorBlob->GetBufferPointer();
        ServiceProvider::getVSLogger()->print<Severity::Error>(str.str().c_str());
        ThrowIfFailed(hr);
        return false;
    }

    hr = device->CreateRootSignature(
        0,
        serializedRootSignature->GetBufferPointer(),
        serializedRootSignature->GetBufferSize(),
        IID_PPV_ARGS(mMainRootSignature.GetAddressOf())
        );

    if (hr != S_OK)
    {
        ServiceProvider::getVSLogger()->print<Severity::Error>("Error creating root signature!");
        ThrowIfFailed(hr);
        return false;
    }

    return true;
}

bool RenderResource::buildDescriptorHeap()
{
    UINT texIndex = 0;

    /*SRV heap description*/
    D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
    srvHeapDesc.NumDescriptors = (UINT)mTextures.size();
    srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

    ThrowIfFailed(device->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&mSrvDescriptorHeap)));

    /*fill the descriptor*/
    CD3DX12_CPU_DESCRIPTOR_HANDLE handleDescriptor(mSrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

    /*first add all Textures2D*/
    for (auto const& t : mTextures)
    {
        if (t.second->Type != TextureType::Texture2D) continue;
        t.second->index = texIndex;
        srvDesc.Format = t.second->Resource->GetDesc().Format;
        srvDesc.Texture2D.MipLevels = t.second->Resource->GetDesc().MipLevels;
        device->CreateShaderResourceView(t.second->Resource.Get(), &srvDesc, handleDescriptor);

        handleDescriptor.Offset(1, mHeapDescriptorSize);

        texIndex++;
    }

    /*after that all TextureCubes*/
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
    srvDesc.TextureCube.MostDetailedMip = 0;
    srvDesc.TextureCube.ResourceMinLODClamp = 0.0f;

    for (auto const& t : mTextures)
    {
        if (t.second->Type != TextureType::TextureCube)continue;
        t.second->index = texIndex;
        srvDesc.Format = t.second->Resource->GetDesc().Format;
        srvDesc.TextureCube.MipLevels = t.second->Resource->GetDesc().MipLevels;
        device->CreateShaderResourceView(t.second->Resource.Get(), &srvDesc, handleDescriptor);

        handleDescriptor.Offset(1, mHeapDescriptorSize);
        texIndex++;
    }


    return true;
}

std::array<const CD3DX12_STATIC_SAMPLER_DESC, 6> RenderResource::GetStaticSamplers()
{
    const CD3DX12_STATIC_SAMPLER_DESC pointWrap(
        0, // shaderRegister
        D3D12_FILTER_MIN_MAG_MIP_POINT, // filter
        D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
        D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
        D3D12_TEXTURE_ADDRESS_MODE_WRAP); // addressW

    const CD3DX12_STATIC_SAMPLER_DESC pointClamp(
        1, // shaderRegister
        D3D12_FILTER_MIN_MAG_MIP_POINT, // filter
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP); // addressW

    const CD3DX12_STATIC_SAMPLER_DESC linearWrap(
        2, // shaderRegister
        D3D12_FILTER_MIN_MAG_MIP_LINEAR, // filter
        D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
        D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
        D3D12_TEXTURE_ADDRESS_MODE_WRAP); // addressW

    const CD3DX12_STATIC_SAMPLER_DESC linearClamp(
        3, // shaderRegister
        D3D12_FILTER_MIN_MAG_MIP_LINEAR, // filter
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP); // addressW

    const CD3DX12_STATIC_SAMPLER_DESC anisotropicWrap(
        4, // shaderRegister
        D3D12_FILTER_ANISOTROPIC, // filter
        D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
        D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
        D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressW
        0.0f,                             // mipLODBias
        8);                               // maxAnisotropy

    const CD3DX12_STATIC_SAMPLER_DESC anisotropicClamp(
        5, // shaderRegister
        D3D12_FILTER_ANISOTROPIC, // filter
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressW
        0.0f,                              // mipLODBias
        8);                                // maxAnisotropy

    return {
        pointWrap, pointClamp,
        linearWrap, linearClamp,
        anisotropicWrap, anisotropicClamp };
}

void RenderResource::buildShaders()
{
    
    const D3D_SHADER_MACRO alphaTestDefines[] =
    {
        "ALPHA_TEST", "1",
        NULL, NULL
    };


    mShaders["defaultVS"] = d3dUtil::CompileShader(L"shader\\default.hlsl", nullptr, "VS", "vs_5_1");
    mShaders["defaultPS"] = d3dUtil::CompileShader(L"shader\\default.hlsl", nullptr, "PS", "ps_5_1");

    mShaders["skyVS"] = d3dUtil::CompileShader(L"shader\\skysphere.hlsl", nullptr, "VS", "vs_5_1");
    mShaders["skyPS"] = d3dUtil::CompileShader(L"shader\\skysphere.hlsl", nullptr, "PS", "ps_5_1");
}

void RenderResource::buildInputLayouts()
{

    mInputLayouts["default"] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 32, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };

}

void RenderResource::generateDefaultShapes()
{
    GeoGenerator geo;

    GeoGenerator::MeshData box = geo.createBoxMesh(1.0f, 1.0f, 1.0f, 0);
    GeoGenerator::MeshData grid = geo.createGrid(10.f, 10.f, 10, 10);
    GeoGenerator::MeshData sphere = geo.createSphereMesh(0.5f, 16, 16);
    
    /*all default shapes in one vertex buffer*/

    UINT boxVertexOffset = 0;
    UINT gridVertexOffset = (UINT)box.Vertices.size();
    UINT sphereVertexOffset = (UINT)gridVertexOffset + (UINT)grid.Vertices.size();

    UINT boxIndexOffset = 0;
    UINT gridIndexOffset = (UINT)box.Indices32.size();
    UINT sphereIndexOffset = gridIndexOffset + (UINT)grid.Indices32.size();

    SubMesh boxSubmesh;
    boxSubmesh.BaseVertexLocation = boxVertexOffset;
    boxSubmesh.StartIndexLocation = boxIndexOffset;
    boxSubmesh.IndexCount = (UINT)box.Indices32.size();

    SubMesh gridSubmesh;
    gridSubmesh.BaseVertexLocation = gridVertexOffset;
    gridSubmesh.StartIndexLocation = gridIndexOffset;
    gridSubmesh.IndexCount = (UINT)grid.Indices32.size();

    SubMesh sphereSubmesh;
    sphereSubmesh.BaseVertexLocation = sphereVertexOffset;
    sphereSubmesh.StartIndexLocation = sphereIndexOffset;
    sphereSubmesh.IndexCount = (UINT)sphere.Indices32.size();

    size_t totalVertices = box.Vertices.size() + grid.Vertices.size() + sphere.Vertices.size();


    std::vector<Vertex> vertices(totalVertices);

    UINT k = 0;
    for (size_t i = 0; i < box.Vertices.size(); i++, k++)
    {
        vertices[k].Pos = box.Vertices[i].Position;
        vertices[k].Normal = box.Vertices[i].Normal;
        vertices[k].TexC = box.Vertices[i].TexC;
        vertices[k].TangentU = box.Vertices[i].TangentU;
    }

    for (size_t i = 0; i < box.Vertices.size(); i++, k++)
    {
        vertices[k].Pos = grid.Vertices[i].Position;
        vertices[k].Normal = grid.Vertices[i].Normal;
        vertices[k].TexC = grid.Vertices[i].TexC;
        vertices[k].TangentU = grid.Vertices[i].TangentU;
    }

    for (size_t i = 0; i < box.Vertices.size(); i++, k++)
    {
        vertices[k].Pos = sphere.Vertices[i].Position;
        vertices[k].Normal = sphere.Vertices[i].Normal;
        vertices[k].TexC = sphere.Vertices[i].TexC;
        vertices[k].TangentU = sphere.Vertices[i].TangentU;
    }

    std::vector<unsigned short> indices;
    indices.insert(indices.end(), std::begin(box.GetIndices16()), std::end(box.GetIndices16()));
    indices.insert(indices.end(), std::begin(grid.GetIndices16()), std::end(grid.GetIndices16()));
    indices.insert(indices.end(), std::begin(sphere.GetIndices16()), std::end(sphere.GetIndices16()));


    /*buffersize in bytes*/
    const UINT vertexByteSize = (UINT)vertices.size() * sizeof(Vertex);
    const UINT indexByteSize = (UINT)indices.size() * sizeof(unsigned short);

    auto geoMesh = std::make_unique<Mesh>();
    geoMesh->name = "default";

    ThrowIfFailed(D3DCreateBlob(vertexByteSize, &geoMesh->VertexBufferCPU));
    CopyMemory(geoMesh->VertexBufferCPU->GetBufferPointer(), vertices.data(), vertexByteSize);

    ThrowIfFailed(D3DCreateBlob(indexByteSize, &geoMesh->IndexBufferCPU));
    CopyMemory(geoMesh->IndexBufferCPU->GetBufferPointer(), indices.data(), indexByteSize);

    geoMesh->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(device, cmdList, vertices.data(), vertexByteSize, geoMesh->VertexBufferUploader);
    geoMesh->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(device, cmdList, indices.data(), indexByteSize, geoMesh->IndexBufferUploader);


    geoMesh->VertexByteStride = sizeof(Vertex);
    geoMesh->VertexBufferByteSize = vertexByteSize;
    geoMesh->IndexFormat = DXGI_FORMAT_R16_UINT;
    geoMesh->IndexBufferByteSize = indexByteSize;

    geoMesh->DrawArgs["box"] = boxSubmesh;
    geoMesh->DrawArgs["grid"] = gridSubmesh;
    geoMesh->DrawArgs["sphere"] = sphereSubmesh;

    mMeshes[geoMesh->name] = std::move(geoMesh);
}


void RenderResource::buildPSOs()
{
    /*default PSO*/
    D3D12_GRAPHICS_PIPELINE_STATE_DESC defaultPSODesc;

    ZeroMemory(&defaultPSODesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
    defaultPSODesc.InputLayout = { mInputLayouts["default"].data(), (UINT)mInputLayouts["default"].size() };
    defaultPSODesc.pRootSignature = mMainRootSignature.Get();
    defaultPSODesc.VS =
    {
        reinterpret_cast<BYTE*>(mShaders["defaultVS"]->GetBufferPointer()),
        mShaders["defaultVS"]->GetBufferSize()
    };
    defaultPSODesc.PS =
    {
        reinterpret_cast<BYTE*>(mShaders["defaultPS"]->GetBufferPointer()),
        mShaders["defaultPS"]->GetBufferSize()
    };
    defaultPSODesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    defaultPSODesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    defaultPSODesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    defaultPSODesc.SampleMask = UINT_MAX;
    defaultPSODesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    defaultPSODesc.NumRenderTargets = 1;
    defaultPSODesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    defaultPSODesc.SampleDesc.Count = 1;
    defaultPSODesc.SampleDesc.Quality =  0;
    defaultPSODesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
    ThrowIfFailed(device->CreateGraphicsPipelineState(&defaultPSODesc, IID_PPV_ARGS(&mPSOs["default"])));

    /*sky sphere PSO*/
    D3D12_GRAPHICS_PIPELINE_STATE_DESC skyPSODesc = defaultPSODesc;

    skyPSODesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
    skyPSODesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
    skyPSODesc.VS =
    {
        reinterpret_cast<BYTE*>(mShaders["skyVS"]->GetBufferPointer()),
        mShaders["skyVS"]->GetBufferSize()
    };
    skyPSODesc.PS =
    {
        reinterpret_cast<BYTE*>(mShaders["skyPS"]->GetBufferPointer()),
        mShaders["skyPS"]->GetBufferSize()
    };
    ThrowIfFailed(device->CreateGraphicsPipelineState(&skyPSODesc, IID_PPV_ARGS(&mPSOs["sky"])));

}

bool RenderResource::buildMaterials()
{
    std::ifstream inputJson;
    json matData;

    try
    {
        inputJson.open("data/materials/mat.json");
        matData = json::parse(inputJson);
        inputJson.close();
    }
    catch (...)
    {
        ServiceProvider::getVSLogger()->print<Severity::Error>("Failed to parse material file");
        return false;
    }

    int matCounter = 0;

    for (auto const& i : matData["Materials"])
    {
        auto material = std::make_unique<Material>();
        material->Name = i["Name"];
        material->MaterialCBIndex = matCounter;

        material->DiffuseAlbedo = XMFLOAT4(i["DiffuseAlbedo"][0], i["DiffuseAlbedo"][1],
                                 i["DiffuseAlbedo"][2], i["DiffuseAlbedo"][3]);
        material->FresnelR0 = XMFLOAT3(i["FresnelR0"][0], i["FresnelR0"][1], i["FresnelR0"][2]);
        material->Roughness = i["Roughness"];

        std::string texName = std::string(i["Texture"]) + ".dds";
        std::string norName = std::string(i["NormalMap"]) + ".dds";

        if (mTextures.find(texName) == mTextures.end() || mTextures.find(norName) == mTextures.end())
        {
            std::stringstream str;
            str << "Can't create material " << material->Name << " due to missing textures!";
            ServiceProvider::getVSLogger()->print<Severity::Critical>(str.str().c_str());
            return false;
        }

        material->DiffuseSrvHeapIndex = mTextures[texName]->index;
        material->NormalSrvHeapIndex = mTextures[norName]->index;
        mMaterials[material->Name] = std::move(material);

        matCounter++;
    }

    return true;
}

void RenderResource::buildRenderItems()
{

    //auto boxRitem = std::make_unique<RenderItem>();
    //XMStoreFloat4x4(&boxRitem->World, XMMatrixScaling(2.0f, 1.0f, 2.0f) * XMMatrixTranslation(0.0f, 0.5f, 0.0f));
    //XMStoreFloat4x4(&boxRitem->TexTransform, XMMatrixScaling(1.0f, 0.5f, 1.0f));
    //boxRitem->ObjCBIndex = 1;
    //boxRitem->Mat = mMaterials["bricks0"].get();
    //boxRitem->Geo = mGeometries["shapeGeo"].get();
    //boxRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    //boxRitem->IndexCount = boxRitem->Geo->DrawArgs["box"].IndexCount;
    //boxRitem->StartIndexLocation = boxRitem->Geo->DrawArgs["box"].StartIndexLocation;
    //boxRitem->BaseVertexLocation = boxRitem->Geo->DrawArgs["box"].BaseVertexLocation;

    //mRitemLayer[(int)RenderLayer::Opaque].push_back(boxRitem.get());
    //mAllRitems.push_back(std::move(boxRitem));

}
