#include "renderresource.h"
#include "../util/modelloader.h"
#include "../util/serviceprovider.h"

bool RenderResource::init(ID3D12Device* _device, ID3D12GraphicsCommandList* _cmdList, const std::filesystem::path& _texturePath, const std::filesystem::path& _modelPath)
{
    device = _device;
    cmdList = _cmdList;

    int textureCounter = 0;
    int modelCounter = 0;

    mCbvSrvUavDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    mRtvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    mDsvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

    mShadowMap = std::make_unique<ShadowMap>(device,
                                             ServiceProvider::getSettings()->graphicSettings.ShadowQuality,
                                             ServiceProvider::getSettings()->graphicSettings.ShadowQuality,
                                             40);

    mRenderTarget = std::make_unique<RenderTarget>(
        device,
        ServiceProvider::getSettings()->displaySettings.ResolutionWidth, 
        ServiceProvider::getSettings()->displaySettings.ResolutionHeight,
        DXGI_FORMAT_R8G8B8A8_UNORM);

    mSobelFilter = std::make_unique<Sobel>(
        device,
        ServiceProvider::getSettings()->displaySettings.ResolutionWidth,
        ServiceProvider::getSettings()->displaySettings.ResolutionHeight,
        DXGI_FORMAT_R8G8B8A8_UNORM);

    buildShaders();
    buildRootSignature();
    buildPostProcessSignature();
    buildInputLayouts();

    /*load all textures*/

    const UINT texTypes = 2;
    std::stringstream tstr[texTypes];
    tstr[0] << _texturePath.string() << "/texture2d";
    tstr[1] << _texturePath.string() << "/texturecube";

    int texCounter[texTypes] = { 0 };

    int tC = 0;
    int texTotal = 0;

    for (auto const& s : tstr)
    {
        for (const auto& entry : std::filesystem::recursive_directory_iterator(std::filesystem::path(s.str())))
        {
            texTotal++;

            if (loadTexture(entry, static_cast<TextureType>(tC)))
            {
                textureCounter++;
                texCounter[tC]++;
            }
            else
            {
                LOG(Severity::Warning, "Failed to load texture " << entry.path().u8string() << "!");
            }
        }
        tC++;
    }

    buildDescriptorHeap();

    /*list loaded textures*/
    std::stringstream str;
    str << "Successfully loaded " << textureCounter << "/" << texTotal << " textures. (";
    for (UINT i = 0; i < texTypes; i++)
    {
        str << texCounter[i];
        if (i != texTypes - 1)
            str << " Texture2D, ";
    }
    str << " TextureCubeMap)";

    ServiceProvider::getLogger()->print<Severity::Info>(str.str().c_str());


    /*load all models*/
    ModelLoader mLoader(device, cmdList);

    for (const auto& entry : std::filesystem::recursive_directory_iterator(_modelPath))
    {
        ModelReturn mRet = mLoader.loadB3D(entry);
        if (mRet.errorCode == 0)
        {
            modelCounter++;
            mModels[entry.path().stem().string()] = std::move(mRet.model);
        }
        else
        {
            LOG(Severity::Warning, "Failed to load model " << entry.path().u8string() << "!");
        }
    }

    LOG(Severity::Info, "Successfully loaded " << modelCounter << " models.");

    /*also generate some default shapes*/

    generateDefaultShapes();

    LOG(Severity::Info, "Successfully created default models.");

    buildPSOs();

    if (!buildMaterials())
    {
        return false;
    }

    buildFrameResource();

    return true;
}

void RenderResource::onResize()
{
}


bool RenderResource::loadTexture(const std::filesystem::directory_entry& file, TextureType type)
{
    auto texMap = std::make_unique<Texture>();
    texMap->Name = file.path().filename().u8string();
    texMap->Filename = AnsiToWString(file.path().u8string());
    texMap->Type = type;

    HRESULT hr = DirectX::CreateDDSTextureFromFile12(device, cmdList, texMap->Filename.c_str(), texMap->Resource, texMap->UploadHeap);

    if (hr == S_OK)
    {
        mTextures[texMap->Name] = std::move(texMap);
        return true;
    }
    else
    {
        return false;
    }
    
}

bool RenderResource::buildRootSignature()
{

    auto staticSamplers = GetStaticSamplers();

    /*main root signature*/


    /*5 root parameter*/
    CD3DX12_ROOT_PARAMETER rootParameter[6];

    /*1 texture in register 0*/
    CD3DX12_DESCRIPTOR_RANGE textureTableReg0;
    textureTableReg0.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0);

    CD3DX12_DESCRIPTOR_RANGE shadowMapReg;
    shadowMapReg.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1, 0);

    /*x textures in register 1*/
    CD3DX12_DESCRIPTOR_RANGE textureTableReg1;
    textureTableReg1.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 128, 2, 0);

    /*constant buffer views in register b0 and b1*/
    rootParameter[0].InitAsConstantBufferView(0);
    rootParameter[1].InitAsConstantBufferView(1);

    /*material data in reg 0 space 1*/
    rootParameter[2].InitAsShaderResourceView(0, 1);

    /*t0,t1 visible for pixel shader*/
    rootParameter[3].InitAsDescriptorTable(1, &shadowMapReg, D3D12_SHADER_VISIBILITY_PIXEL);
    rootParameter[4].InitAsDescriptorTable(1, &textureTableReg0, D3D12_SHADER_VISIBILITY_PIXEL);
    rootParameter[5].InitAsDescriptorTable(1, &textureTableReg1, D3D12_SHADER_VISIBILITY_PIXEL);

    /*get the static samplers and bind them to root signature description*/

    CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDescription(6, rootParameter, (UINT)staticSamplers.size(),
                                                         staticSamplers.data(), D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    /*create root signature using the description*/
    ComPtr<ID3DBlob> serializedRootSignature = nullptr;
    ComPtr<ID3DBlob> errorBlob = nullptr;

    HRESULT hr = D3D12SerializeRootSignature(&rootSignatureDescription, D3D_ROOT_SIGNATURE_VERSION_1,
                                             serializedRootSignature.GetAddressOf(), errorBlob.GetAddressOf());

    if (errorBlob != nullptr)
    {
        LOG(Severity::Error, "Error serializing main root signature: " << (char*)errorBlob->GetBufferPointer());
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
        ServiceProvider::getLogger()->print<Severity::Error>("Error creating main root signature!");
        ThrowIfFailed(hr);
        return false;
    }

    return true;
}

bool RenderResource::buildPostProcessSignature()
{
    ///*sobel root signature*/
    CD3DX12_DESCRIPTOR_RANGE srv0;
    CD3DX12_DESCRIPTOR_RANGE srv1;
    CD3DX12_DESCRIPTOR_RANGE uav0;

    srv0.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
    srv1.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1);
    uav0.Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0);

    CD3DX12_ROOT_PARAMETER slotRootParameter[3];

    slotRootParameter[0].InitAsDescriptorTable(1, &srv0);
    slotRootParameter[1].InitAsDescriptorTable(1, &srv1);
    slotRootParameter[2].InitAsDescriptorTable(1, &uav0);

    auto staticSamplers = GetStaticSamplers();

    CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(3, slotRootParameter,
                                                 (UINT)staticSamplers.size(),
                                                 staticSamplers.data(),
                                                 D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    ComPtr<ID3DBlob> serializedRootSig = nullptr;
    ComPtr<ID3DBlob> errorBlob = nullptr;

    HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1,
                                             serializedRootSig.GetAddressOf(),
                                             errorBlob.GetAddressOf());

    if (errorBlob != nullptr)
    {
        LOG(Severity::Error, "Error serializing sobel root signature: " << (char*)errorBlob->GetBufferPointer());
        ThrowIfFailed(hr);
        return false;
    }

    hr = device->CreateRootSignature(0,
                                     serializedRootSig->GetBufferPointer(),
                                     serializedRootSig->GetBufferSize(),
                                     IID_PPV_ARGS(mPostProcessRootSignature.GetAddressOf()));

    if (hr != S_OK)
    {
        ServiceProvider::getLogger()->print<Severity::Error>("Error creating sobel root signature!");
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
    srvHeapDesc.NumDescriptors = (UINT)mTextures.size()
        + 3 /*shadow map resources*/
        + mSobelFilter->getDescriptorCount() /*sobel filter resource*/
        + 1; /*offscreen rtv*/

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

        handleDescriptor.Offset(1, mCbvSrvUavDescriptorSize);

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

        handleDescriptor.Offset(1, mCbvSrvUavDescriptorSize);
        texIndex++;
    }

    UINT mShadowMapHeapIndex = texIndex++;

    mNullCubeSrvIndex = texIndex++;
    mNullTexSrvIndex = texIndex++;

    auto srvCpuStart = mSrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
    auto srvGpuStart = mSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart();
    auto dsvCpuStart = mDsvHeap->GetCPUDescriptorHandleForHeapStart();
    auto rtvCpuStart = mRtvHeap->GetCPUDescriptorHandleForHeapStart();

    auto nullSrv = CD3DX12_CPU_DESCRIPTOR_HANDLE(srvCpuStart, mNullCubeSrvIndex, mCbvSrvUavDescriptorSize);
    mNullSrv = CD3DX12_GPU_DESCRIPTOR_HANDLE(srvGpuStart, mNullCubeSrvIndex, mCbvSrvUavDescriptorSize);

    device->CreateShaderResourceView(nullptr, &srvDesc, nullSrv);
    nullSrv.Offset(1, mCbvSrvUavDescriptorSize);

    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Texture2D.MipLevels = 1;
    srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
    device->CreateShaderResourceView(nullptr, &srvDesc, nullSrv);

    mShadowMap->BuildDescriptors(
        CD3DX12_CPU_DESCRIPTOR_HANDLE(srvCpuStart, mShadowMapHeapIndex, mCbvSrvUavDescriptorSize),
        CD3DX12_GPU_DESCRIPTOR_HANDLE(srvGpuStart, mShadowMapHeapIndex, mCbvSrvUavDescriptorSize),
        CD3DX12_CPU_DESCRIPTOR_HANDLE(dsvCpuStart, 1, mDsvDescriptorSize));

    UINT sobelOffset = texIndex++;
    texIndex++; /*because sobel uses two slots*/

    mSobelFilter->buildDescriptors(
        CD3DX12_CPU_DESCRIPTOR_HANDLE(srvCpuStart, sobelOffset, mCbvSrvUavDescriptorSize),
        CD3DX12_GPU_DESCRIPTOR_HANDLE(srvGpuStart, sobelOffset, mCbvSrvUavDescriptorSize),
        mCbvSrvUavDescriptorSize);

    UINT offscreenOffset = texIndex++;
    UINT rtvOffset = 2; /*swap chain buffer count*/

    mRenderTarget->buildDescriptors(
        CD3DX12_CPU_DESCRIPTOR_HANDLE(srvCpuStart, offscreenOffset, mCbvSrvUavDescriptorSize),
        CD3DX12_GPU_DESCRIPTOR_HANDLE(srvGpuStart, offscreenOffset, mCbvSrvUavDescriptorSize),
        CD3DX12_CPU_DESCRIPTOR_HANDLE(rtvCpuStart, rtvOffset, device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV))
    );

    return true;
}

std::array<const CD3DX12_STATIC_SAMPLER_DESC, 7> RenderResource::GetStaticSamplers()
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
        ServiceProvider::getSettings()->graphicSettings.AnisotropicFiltering);                               // maxAnisotropy

    const CD3DX12_STATIC_SAMPLER_DESC anisotropicClamp(
        5, // shaderRegister
        D3D12_FILTER_ANISOTROPIC, // filter
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressW
        0.0f,                              // mipLODBias
        ServiceProvider::getSettings()->graphicSettings.AnisotropicFiltering);                                // maxAnisotropy

    const CD3DX12_STATIC_SAMPLER_DESC shadow(
        6,
        D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT,
        D3D12_TEXTURE_ADDRESS_MODE_BORDER,
        D3D12_TEXTURE_ADDRESS_MODE_BORDER,
        D3D12_TEXTURE_ADDRESS_MODE_BORDER,
        0.0f,
        ServiceProvider::getSettings()->graphicSettings.AnisotropicFiltering,
        D3D12_COMPARISON_FUNC_LESS_EQUAL,
        D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE
    );

    return {
        pointWrap, pointClamp,
        linearWrap, linearClamp,
        anisotropicWrap, anisotropicClamp, shadow };
}

#pragma region SHADER

void RenderResource::buildShaders()
{

    const D3D_SHADER_MACRO alphaTestDefines[] =
    {
        "ALPHA_TEST", "1",
        NULL, NULL
    };

    const D3D_SHADER_MACRO normalMapDefines[] = {
        "NORMAL_MAP_DISABLED", "1",
        NULL, NULL
    };

    mShaders["defaultVS"] = d3dUtil::CompileShader(L"data\\shader\\Default.hlsl", nullptr, "VS", "vs_5_1");
    mShaders["defaultPS"] = d3dUtil::CompileShader(L"data\\shader\\Default.hlsl", nullptr, "PS", "ps_5_1");

    mShaders["defaultAlphaVS"] = d3dUtil::CompileShader(L"data\\shader\\Default.hlsl", alphaTestDefines, "VS", "vs_5_1");
    mShaders["defaultAlphaPS"] = d3dUtil::CompileShader(L"data\\shader\\Default.hlsl", alphaTestDefines, "PS", "ps_5_1");

    mShaders["defaultNoNormalVS"] = d3dUtil::CompileShader(L"data\\shader\\Default.hlsl", normalMapDefines, "VS", "vs_5_1");
    mShaders["defaultNoNormalPS"] = d3dUtil::CompileShader(L"data\\shader\\Default.hlsl", normalMapDefines, "PS", "ps_5_1");


    mShaders["skyVS"] = d3dUtil::CompileShader(L"data\\shader\\Sky.hlsl", nullptr, "VS", "vs_5_1");
    mShaders["skyPS"] = d3dUtil::CompileShader(L"data\\shader\\Sky.hlsl", nullptr, "PS", "ps_5_1");

    mShaders["hitboxVS"] = d3dUtil::CompileShader(L"data\\shader\\Hitbox.hlsl", nullptr, "VS", "vs_5_1");
    mShaders["hitboxPS"] = d3dUtil::CompileShader(L"data\\shader\\Hitbox.hlsl", nullptr, "PS", "ps_5_1");

    mShaders["shadowVS"] = d3dUtil::CompileShader(L"data\\shader\\Shadows.hlsl", nullptr, "VS", "vs_5_1");
    mShaders["shadowAlphaPS"] = d3dUtil::CompileShader(L"data\\shader\\Shadows.hlsl", nullptr, "PS", "ps_5_1");

    mShaders["compositeVS"] = d3dUtil::CompileShader(L"data\\shader\\Composite.hlsl", nullptr, "VS", "vs_5_1");
    mShaders["compositePS"] = d3dUtil::CompileShader(L"data\\shader\\Composite.hlsl", nullptr, "PS", "ps_5_1");

    mShaders["sobelCS"] = d3dUtil::CompileShader(L"data\\shader\\Sobel.hlsl", nullptr, "CS", "cs_5_1");

    mShaders["debugVS"] = d3dUtil::CompileShader(L"data\\shader\\Debug.hlsl", nullptr, "VS", "vs_5_1");
    mShaders["debugPS"] = d3dUtil::CompileShader(L"data\\shader\\Debug.hlsl", nullptr, "PS", "ps_5_1");


}

#pragma endregion SHADER

void RenderResource::buildInputLayouts()
{

    mInputLayouts.push_back({
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 32, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    });

    //mInputLayouts.push_back(
    //{
    //    { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    //    { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    //    { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    //});

}



#pragma region PSO

void RenderResource::buildPSOs()
{
    /*default PSO*/
    D3D12_GRAPHICS_PIPELINE_STATE_DESC defaultPSODesc;

    ZeroMemory(&defaultPSODesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
    defaultPSODesc.InputLayout = { mInputLayouts[0].data(), (UINT)mInputLayouts[0].size() };
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
    ThrowIfFailed(device->CreateGraphicsPipelineState(&defaultPSODesc, IID_PPV_ARGS(&mPSOs[RenderType::Default])));

    /* default alpha PSO*/
    D3D12_GRAPHICS_PIPELINE_STATE_DESC defaultAlphaDesc = defaultPSODesc;

    defaultAlphaDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;

    defaultAlphaDesc.VS =
    {
        reinterpret_cast<BYTE*>(mShaders["defaultAlphaVS"]->GetBufferPointer()),
        mShaders["defaultAlphaVS"]->GetBufferSize()
    };
    defaultAlphaDesc.PS =
    {
        reinterpret_cast<BYTE*>(mShaders["defaultAlphaPS"]->GetBufferPointer()),
        mShaders["defaultAlphaPS"]->GetBufferSize()
    };

    ThrowIfFailed(device->CreateGraphicsPipelineState(&defaultAlphaDesc, IID_PPV_ARGS(&mPSOs[RenderType::DefaultAlpha])));


    /* default no normal mapping PSO */
    D3D12_GRAPHICS_PIPELINE_STATE_DESC defaultNoNormalDesc = defaultPSODesc;

    defaultNoNormalDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;

    defaultNoNormalDesc.VS =
    {
        reinterpret_cast<BYTE*>(mShaders["defaultNoNormalVS"]->GetBufferPointer()),
        mShaders["defaultNoNormalVS"]->GetBufferSize()
    };
    defaultNoNormalDesc.PS =
    {
        reinterpret_cast<BYTE*>(mShaders["defaultNoNormalPS"]->GetBufferPointer()),
        mShaders["defaultNoNormalPS"]->GetBufferSize()
    };

    ThrowIfFailed(device->CreateGraphicsPipelineState(&defaultNoNormalDesc, IID_PPV_ARGS(&mPSOs[RenderType::DefaultNoNormal])));

    /* Transparency PSO*/
    D3D12_GRAPHICS_PIPELINE_STATE_DESC transparencyPSODesc = defaultPSODesc;

    D3D12_RENDER_TARGET_BLEND_DESC transparencyBlendDesc;
    transparencyBlendDesc.BlendEnable = true;
    transparencyBlendDesc.LogicOpEnable = false;
    transparencyBlendDesc.SrcBlend = D3D12_BLEND_SRC_ALPHA;
    transparencyBlendDesc.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
    transparencyBlendDesc.BlendOp = D3D12_BLEND_OP_ADD;
    transparencyBlendDesc.SrcBlendAlpha = D3D12_BLEND_ONE;
    transparencyBlendDesc.DestBlendAlpha = D3D12_BLEND_ZERO;
    transparencyBlendDesc.BlendOpAlpha = D3D12_BLEND_OP_ADD;
    transparencyBlendDesc.LogicOp = D3D12_LOGIC_OP_NOOP;
    transparencyBlendDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

    transparencyPSODesc.BlendState.RenderTarget[0] = transparencyBlendDesc;

    ThrowIfFailed(device->CreateGraphicsPipelineState(&transparencyPSODesc, IID_PPV_ARGS(&mPSOs[RenderType::DefaultTransparency])));

    /*shadow pass PSO*/
    D3D12_GRAPHICS_PIPELINE_STATE_DESC smapPsoDesc = defaultPSODesc;
    smapPsoDesc.RasterizerState.DepthBias = 10000;
    smapPsoDesc.RasterizerState.DepthBiasClamp = 0.0f;
    smapPsoDesc.RasterizerState.SlopeScaledDepthBias = 1.0f;
    smapPsoDesc.pRootSignature = mMainRootSignature.Get();
    smapPsoDesc.VS =
    {
        reinterpret_cast<BYTE*>(mShaders["shadowVS"]->GetBufferPointer()),
        mShaders["shadowVS"]->GetBufferSize()
    };
    smapPsoDesc.PS =
    {
        nullptr,
        0
    };

    smapPsoDesc.RTVFormats[0] = DXGI_FORMAT_UNKNOWN;
    smapPsoDesc.NumRenderTargets = 0;
    ThrowIfFailed(device->CreateGraphicsPipelineState(&smapPsoDesc, IID_PPV_ARGS(&mPSOs[RenderType::ShadowDefault])));

    /*shadow alpha PSO*/

    D3D12_GRAPHICS_PIPELINE_STATE_DESC smapAlphaPsoDesc = smapPsoDesc;
    smapAlphaPsoDesc.PS =
    {
        reinterpret_cast<BYTE*>(mShaders["shadowAlphaPS"]->GetBufferPointer()),
        mShaders["shadowAlphaPS"]->GetBufferSize()
    };

    ThrowIfFailed(device->CreateGraphicsPipelineState(&smapAlphaPsoDesc, IID_PPV_ARGS(&mPSOs[RenderType::ShadowAlpha])));


    /*debug PSO*/
    D3D12_GRAPHICS_PIPELINE_STATE_DESC debugPsoDesc = defaultPSODesc;
    debugPsoDesc.pRootSignature = mMainRootSignature.Get();
    debugPsoDesc.VS =
    {
        reinterpret_cast<BYTE*>(mShaders["debugVS"]->GetBufferPointer()),
        mShaders["debugVS"]->GetBufferSize()
    };
    debugPsoDesc.PS =
    {
        reinterpret_cast<BYTE*>(mShaders["debugPS"]->GetBufferPointer()),
        mShaders["debugPS"]->GetBufferSize()
    };
    ThrowIfFailed(device->CreateGraphicsPipelineState(&debugPsoDesc, IID_PPV_ARGS(&mPSOs[RenderType::Debug])));

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
    ThrowIfFailed(device->CreateGraphicsPipelineState(&skyPSODesc, IID_PPV_ARGS(&mPSOs[RenderType::Sky])));


    /*hitbox PSO*/
    D3D12_GRAPHICS_PIPELINE_STATE_DESC hitboxPSODesc = defaultPSODesc;

    hitboxPSODesc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
    hitboxPSODesc.DepthStencilState.DepthEnable = false;
    hitboxPSODesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;

    hitboxPSODesc.VS =
    {
        reinterpret_cast<BYTE*>(mShaders["hitboxVS"]->GetBufferPointer()),
        mShaders["hitboxVS"]->GetBufferSize()
    };
    hitboxPSODesc.PS =
    {
        reinterpret_cast<BYTE*>(mShaders["hitboxPS"]->GetBufferPointer()),
        mShaders["hitboxPS"]->GetBufferSize()
    };
    ThrowIfFailed(device->CreateGraphicsPipelineState(&hitboxPSODesc, IID_PPV_ARGS(&mPSOs[RenderType::Hitbox])));

    /*composite PSO*/
    D3D12_GRAPHICS_PIPELINE_STATE_DESC compositePSO = defaultPSODesc;
    compositePSO.pRootSignature = mPostProcessRootSignature.Get();

    compositePSO.DepthStencilState.DepthEnable = false;
    compositePSO.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
    compositePSO.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_ALWAYS;

    compositePSO.VS = {
        reinterpret_cast<BYTE*>(mShaders["compositeVS"]->GetBufferPointer()),
        mShaders["compositeVS"]->GetBufferSize()
    };

    compositePSO.PS = {
        reinterpret_cast<BYTE*>(mShaders["compositePS"]->GetBufferPointer()),
        mShaders["compositePS"]->GetBufferSize()
    };

    ThrowIfFailed(device->CreateGraphicsPipelineState(&compositePSO, IID_PPV_ARGS(&mPSOs[RenderType::Composite])));


    /*sobel PSO*/
    D3D12_COMPUTE_PIPELINE_STATE_DESC sobelPSO = {};

    sobelPSO.pRootSignature = mPostProcessRootSignature.Get();
    sobelPSO.CS = {
        reinterpret_cast<BYTE*>(mShaders["sobelCS"]->GetBufferPointer()),
        mShaders["sobelCS"]->GetBufferSize()
    };
    sobelPSO.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

    ThrowIfFailed(device->CreateComputePipelineState(&sobelPSO, IID_PPV_ARGS(&mPSOs[RenderType::Sobel])));


}

#pragma endregion PSO

bool RenderResource::buildMaterials()
{
    std::ifstream inputJson;
    json matData;

    try
    {
        inputJson.open("data/material/mat.json");
        matData = json::parse(inputJson);
        inputJson.close();
    }
    catch (...)
    {
        ServiceProvider::getLogger()->print<Severity::Error>("Failed to parse material file");
        return false;
    }

    int matCounter = 0;

    for (auto const& i : matData["Materials"])
    {
        auto material = std::make_unique<Material>();
        material->Name = i["Name"];
        material->MatCBIndex = matCounter;

        material->DiffuseAlbedo = XMFLOAT4(i["DiffuseAlbedo"][0], i["DiffuseAlbedo"][1],
                                 i["DiffuseAlbedo"][2], i["DiffuseAlbedo"][3]);
        material->FresnelR0 = XMFLOAT3(i["FresnelR0"][0], i["FresnelR0"][1], i["FresnelR0"][2]);
        material->Roughness = i["Roughness"];

        std::string texName = std::string(i["Texture"]) + ".dds";
        std::string norName = std::string(i["NormalMap"]) + ".dds";

        if (mTextures.find(texName) == mTextures.end())
        {
            LOG(Severity::Critical, "Can't create material " << material->Name << " due to missing textures! Using default.");

            texName = "default.dds";
            
        }

        if (mTextures.find(norName) == mTextures.end())
        {
            material->usesNormalMapping = false;
            norName = "default_nmap.dds";
        }

        material->DiffuseSrvHeapIndex = mTextures[texName]->index;
        material->NormalSrvHeapIndex = mTextures[norName]->index;
        mMaterials[material->Name] = std::move(material);

        matCounter++;
    }

    return true;
}



void RenderResource::updateBuffers(const GameTime& gt)
{
    static bool once = [&]()
    {
        mLightRotationAngle = 0.0f;

        XMMATRIX R = DirectX::XMMatrixRotationY(mLightRotationAngle);
        for (int i = 0; i < 3; ++i)
        {
            XMVECTOR lightDir = XMLoadFloat3(&mBaseLightDirections[i]);
            lightDir = XMVector3TransformNormal(lightDir, R);
            XMStoreFloat3(&mRotatedLightDirections[i], lightDir);
        }
        return true;
    } ();
    
    updateGameObjectConstantBuffers(gt);
    updateMaterialConstantBuffers(gt);
    updateShadowTransform(gt);
    updateMainPassConstantBuffers(gt);
    updateShadowPassConstantBuffers(gt);
}

void RenderResource::setPSO(RenderType renderType)
{
    cmdList->SetPipelineState(mPSOs[renderType].Get());
}

ID3D12PipelineState* RenderResource::getPSO(RenderType renderType)
{
    return mPSOs[renderType].Get();
}

void RenderResource::updateShadowTransform(const GameTime& gt)
{
    // Only the first "main" light casts a shadow.
    XMVECTOR lightDir = XMLoadFloat3(&mRotatedLightDirections[0]);
    XMVECTOR lightPos = -2.0f * mShadowMap->shadowBounds.Radius * lightDir + XMLoadFloat3(&mShadowMap->shadowBounds.Center);
    XMVECTOR targetPos = XMLoadFloat3(&mShadowMap->shadowBounds.Center);
    XMVECTOR lightUp = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    XMMATRIX lightView = XMMatrixLookAtLH(lightPos, targetPos, lightUp);

    XMStoreFloat3(&mLightPosW, lightPos);

    // Transform bounding sphere to light space.
    XMFLOAT3 sphereCenterLS;
    XMStoreFloat3(&sphereCenterLS, XMVector3TransformCoord(targetPos, lightView));

    // Ortho frustum in light space encloses scene.
    float l = sphereCenterLS.x - mShadowMap->shadowBounds.Radius;
    float b = sphereCenterLS.y - mShadowMap->shadowBounds.Radius;
    float n = sphereCenterLS.z - mShadowMap->shadowBounds.Radius;
    float r = sphereCenterLS.x + mShadowMap->shadowBounds.Radius;
    float t = sphereCenterLS.y + mShadowMap->shadowBounds.Radius;
    float f = sphereCenterLS.z + mShadowMap->shadowBounds.Radius;

    mLightNearZ = n;
    mLightFarZ = f;
    XMMATRIX lightProj = XMMatrixOrthographicOffCenterLH(l, r, b, t, n, f);

    // Transform NDC space [-1,+1]^2 to texture space [0,1]^2
    XMMATRIX T(
        0.5f, 0.0f, 0.0f, 0.0f,
        0.0f, -0.5f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.5f, 0.5f, 0.0f, 1.0f);

    XMMATRIX S = lightView * lightProj * T;
    XMStoreFloat4x4(&mLightView, lightView);
    XMStoreFloat4x4(&mLightProj, lightProj);
    XMStoreFloat4x4(&mShadowTransform, S);
}

void RenderResource::updateGameObjectConstantBuffers(const GameTime& gt)
{

    auto currObjectCB = mCurrentFrameResource->ObjectCB.get();

    for (auto& go : ServiceProvider::getActiveLevel()->mGameObjects)
    {
        auto e = go.second->renderItem.get();

        // Only update the cbuffer data if the constants have changed.  
        if (e->NumFramesDirty > 0)
        {
            XMMATRIX world = XMLoadFloat4x4(&e->World);
            XMMATRIX texTransform = XMLoadFloat4x4(&e->TexTransform);

            ObjectConstants objConstants;
            XMStoreFloat4x4(&objConstants.World, XMMatrixTranspose(world));
            XMStoreFloat4x4(&objConstants.TexTransform, XMMatrixTranspose(texTransform));
            objConstants.MaterialIndex = e->Mat->MatCBIndex;

            currObjectCB->copyData(e->ObjCBIndex, objConstants);

            // Next FrameResource need to be updated too.
            e->NumFramesDirty--;
        }
    }

}


void RenderResource::updateMaterialConstantBuffers(const GameTime& gt)
{

    auto currMaterialBuffer = mCurrentFrameResource->MaterialBuffer.get();
    for (auto& e : ServiceProvider::getRenderResource()->mMaterials)
    {
        Material* mat = e.second.get();
        if (mat->NumFramesDirty > 0)
        {
            XMMATRIX matTransform = XMLoadFloat4x4(&mat->MatTransform);

            MaterialData matData;
            matData.DiffuseAlbedo = mat->DiffuseAlbedo;
            matData.FresnelR0 = mat->FresnelR0;
            matData.Roughness = mat->Roughness;
            XMStoreFloat4x4(&matData.MatTransform, XMMatrixTranspose(matTransform));
            matData.DiffuseMapIndex = mat->DiffuseSrvHeapIndex;
            matData.NormalMapIndex = mat->NormalSrvHeapIndex;

            currMaterialBuffer->copyData(mat->MatCBIndex, matData);

            mat->NumFramesDirty--;
        }
    }

}

void RenderResource::updateShadowPassConstantBuffers(const GameTime& gt)
{

    XMMATRIX view = XMLoadFloat4x4(&mLightView);
    XMMATRIX proj = XMLoadFloat4x4(&mLightProj);

    XMMATRIX viewProj = XMMatrixMultiply(view, proj);
    XMMATRIX invView = XMMatrixInverse(&XMMatrixDeterminant(view), view);
    XMMATRIX invProj = XMMatrixInverse(&XMMatrixDeterminant(proj), proj);
    XMMATRIX invViewProj = XMMatrixInverse(&XMMatrixDeterminant(viewProj), viewProj);

    UINT w = mShadowMap->Width();
    UINT h = mShadowMap->Height();

    XMStoreFloat4x4(&mShadowPassConstants.View, XMMatrixTranspose(view));
    XMStoreFloat4x4(&mShadowPassConstants.InvView, XMMatrixTranspose(invView));
    XMStoreFloat4x4(&mShadowPassConstants.Proj, XMMatrixTranspose(proj));
    XMStoreFloat4x4(&mShadowPassConstants.InvProj, XMMatrixTranspose(invProj));
    XMStoreFloat4x4(&mShadowPassConstants.ViewProj, XMMatrixTranspose(viewProj));
    XMStoreFloat4x4(&mShadowPassConstants.InvViewProj, XMMatrixTranspose(invViewProj));
    mShadowPassConstants.EyePosW = mLightPosW;
    mShadowPassConstants.RenderTargetSize = XMFLOAT2((float)w, (float)h);
    mShadowPassConstants.InvRenderTargetSize = XMFLOAT2(1.0f / w, 1.0f / h);
    mShadowPassConstants.NearZ = mLightNearZ;
    mShadowPassConstants.FarZ = mLightFarZ;

    auto currPassCB = mCurrentFrameResource->PassCB.get();
    currPassCB->copyData(1, mShadowPassConstants);

}


void RenderResource::updateMainPassConstantBuffers(const GameTime& gt)
{
    /*always update the camera light etc buffer*/
    XMMATRIX view = ServiceProvider::getActiveCamera()->getView();
    XMMATRIX proj = ServiceProvider::getActiveCamera()->getProj();

    XMMATRIX viewProj = XMMatrixMultiply(view, proj);
    XMMATRIX invView = XMMatrixInverse(&XMMatrixDeterminant(view), view);
    XMMATRIX invProj = XMMatrixInverse(&XMMatrixDeterminant(proj), proj);
    XMMATRIX invViewProj = XMMatrixInverse(&XMMatrixDeterminant(viewProj), viewProj);

    XMStoreFloat4x4(&mMainPassConstants.View, XMMatrixTranspose(view));
    XMStoreFloat4x4(&mMainPassConstants.InvView, XMMatrixTranspose(invView));
    XMStoreFloat4x4(&mMainPassConstants.Proj, XMMatrixTranspose(proj));
    XMStoreFloat4x4(&mMainPassConstants.InvProj, XMMatrixTranspose(invProj));
    XMStoreFloat4x4(&mMainPassConstants.ViewProj, XMMatrixTranspose(viewProj));
    XMStoreFloat4x4(&mMainPassConstants.InvViewProj, XMMatrixTranspose(invViewProj));
    XMStoreFloat4x4(&mMainPassConstants.ShadowTransform, XMMatrixTranspose(XMLoadFloat4x4(&mShadowTransform)));
    mMainPassConstants.EyePosW = ServiceProvider::getActiveCamera()->getPosition3f();



    mMainPassConstants.RenderTargetSize = XMFLOAT2((float)ServiceProvider::getSettings()->displaySettings.ResolutionWidth, (float)ServiceProvider::getSettings()->displaySettings.ResolutionHeight);
    mMainPassConstants.InvRenderTargetSize = XMFLOAT2(1.0f / ServiceProvider::getSettings()->displaySettings.ResolutionWidth, 1.0f / ServiceProvider::getSettings()->displaySettings.ResolutionHeight);
    mMainPassConstants.NearZ = 0.01f;
    mMainPassConstants.FarZ = 1000.0f;
    mMainPassConstants.TotalTime = gt.TotalTime();
    mMainPassConstants.DeltaTime = gt.DeltaTime();

    /*TODO*/
    mMainPassConstants.AmbientLight = { 0.25f, 0.25f, 0.35f, 1.0f };
    mMainPassConstants.Lights[0].Direction = { 0.57735f, -0.57735f, 0.57735f };
    mMainPassConstants.Lights[0].Strength = { 0.8f, 0.8f, 0.8f };
    mMainPassConstants.Lights[1].Direction = { -0.57735f, -0.57735f, 0.57735f };
    mMainPassConstants.Lights[1].Strength = { 0.4f, 0.4f, 0.4f };
    mMainPassConstants.Lights[2].Direction = { 0.0f, -0.707f, -0.707f };
    mMainPassConstants.Lights[2].Strength = { 0.2f, 0.2f, 0.2f };

    auto currPassCB = mCurrentFrameResource->PassCB.get();
    currPassCB->copyData(0, mMainPassConstants);

}

/*Frame Resource*/
void RenderResource::buildFrameResource()
{

    for (int i = 0; i < gNumFrameResources; i++)
    {
        mFrameResources.push_back(std::make_unique<FrameResource>(device,
                                  2, /*main pass and shadow pass cbs*/
                                  MAX_GAME_OBJECTS,
                                  0,
                                  (UINT)mMaterials.size()));
    }

}

void RenderResource::cycleFrameResource()
{
    mCurrentFrameResourceIndex = (mCurrentFrameResourceIndex + 1) % gNumFrameResources;
    mCurrentFrameResource = mFrameResources[mCurrentFrameResourceIndex].get();
}

int RenderResource::getCurrentFrameResourceIndex()
{
    return mCurrentFrameResourceIndex;
}

FrameResource* RenderResource::getCurrentFrameResource()
{
    return mCurrentFrameResource;
}
/***************/

void RenderResource::generateDefaultShapes()
{
    /*create the shapes*/
    GeometryGenerator geoGen;
    GeometryGenerator::MeshData box = geoGen.CreateBox(1.0f, 1.0f, 1.0f, 0);
    GeometryGenerator::MeshData grid = geoGen.CreateGrid(10.0f, 10.0f, 10, 10);
    GeometryGenerator::MeshData sphere = geoGen.CreateSphere(1.0f, 32, 32);
    GeometryGenerator::MeshData cylinder = geoGen.CreateCylinder(1.0f, 1.0f, 2.0f, 32, 32);
    GeometryGenerator::MeshData quad = geoGen.CreateQuad(0.f, 0.f, 0.5f, 0.5f, 0.0f);

    /*copy to box model*/
    std::vector<Vertex> vertices(box.Vertices.size());
    std::vector<std::uint16_t> indices(box.Indices32.size());

    XMFLOAT3 cMin(+FLT_MAX, +FLT_MAX, +FLT_MAX);
    XMFLOAT3 cMax(-FLT_MAX, -FLT_MAX, -FLT_MAX);

    XMVECTOR vMin = XMLoadFloat3(&cMin);
    XMVECTOR vMax = XMLoadFloat3(&cMax);

    for (size_t i = 0; i < box.Vertices.size(); i++)
    {
        vertices[i].Pos = box.Vertices[i].Position;
        vertices[i].Normal = box.Vertices[i].Normal;
        vertices[i].TexC = box.Vertices[i].TexC;
        vertices[i].TangentU = box.Vertices[i].TangentU;

        XMVECTOR P = XMLoadFloat3(&vertices[i].Pos);

        vMin = XMVectorMin(vMin, P);
        vMax = XMVectorMax(vMax, P);
    }

    indices.insert(indices.end(), std::begin(box.GetIndices16()), std::end(box.GetIndices16()));

    UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);
    UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

    auto geo = std::make_unique<Mesh>();
    geo->name = "box";

    ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU));
    CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

    ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
    CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

    geo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(device,
                                                        cmdList, vertices.data(), vbByteSize, geo->VertexBufferUploader);

    geo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(device,
                                                       cmdList, indices.data(), ibByteSize, geo->IndexBufferUploader);

    geo->VertexByteStride = sizeof(Vertex);
    geo->VertexBufferByteSize = vbByteSize;
    geo->IndexFormat = DXGI_FORMAT_R16_UINT;
    geo->IndexBufferByteSize = ibByteSize;
    geo->IndexCount = (UINT)indices.size();

    std::unique_ptr<Model> m = std::make_unique<Model>();

    m->meshes["box"] = std::move(geo);
    mModels["box"] = std::move(m);

    /*box hitbox*/
    XMStoreFloat3(&mModels["box"]->boundingBox.Center, 0.5f * (vMin + vMax));
    XMStoreFloat3(&mModels["box"]->boundingBox.Extents, 0.5f * (vMax - vMin));

    std::unique_ptr<Mesh> hitboxBox = std::make_unique<Mesh>();

    hitboxBox->name = "hitbox";
    hitboxBox->dTexture = "default";
    hitboxBox->dNormal = "default_nmap";
    hitboxBox->IndexFormat = DXGI_FORMAT_R16_UINT;
    hitboxBox->VertexByteStride = sizeof(Vertex);
    hitboxBox->VertexBufferByteSize = vbByteSize;
    hitboxBox->IndexBufferByteSize = ibByteSize;
    hitboxBox->IndexCount = (UINT)indices.size();

    ThrowIfFailed(D3DCreateBlob(vbByteSize, &hitboxBox->VertexBufferCPU));
    CopyMemory(hitboxBox->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

    ThrowIfFailed(D3DCreateBlob(ibByteSize, &hitboxBox->IndexBufferCPU));
    CopyMemory(hitboxBox->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

    hitboxBox->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(device,
                                                              cmdList, vertices.data(), vbByteSize, hitboxBox->VertexBufferUploader);

    hitboxBox->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(device,
                                                             cmdList, indices.data(), ibByteSize, hitboxBox->IndexBufferUploader);


    mModels["box"]->boundingBoxMesh = std::move(hitboxBox);


    /*quad*/

    vertices.clear();
    vertices.resize(quad.Vertices.size());
    indices.clear();
    indices.resize(quad.Indices32.size());


    for (size_t i = 0; i < quad.Vertices.size(); i++)
    {
        vertices[i].Pos = quad.Vertices[i].Position;
        vertices[i].Normal = quad.Vertices[i].Normal;
        vertices[i].TexC = quad.Vertices[i].TexC;
        vertices[i].TangentU = quad.Vertices[i].TangentU;
    }

    indices.insert(indices.end(), std::begin(quad.GetIndices16()), std::end(quad.GetIndices16()));

    vbByteSize = (UINT)vertices.size() * sizeof(Vertex);
    ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

    auto geoQuad = std::make_unique<Mesh>();
    geoQuad->name = "quad";

    ThrowIfFailed(D3DCreateBlob(vbByteSize, &geoQuad->VertexBufferCPU));
    CopyMemory(geoQuad->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

    ThrowIfFailed(D3DCreateBlob(ibByteSize, &geoQuad->IndexBufferCPU));
    CopyMemory(geoQuad->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

    geoQuad->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(device,
                                                            cmdList, vertices.data(), vbByteSize, geoQuad->VertexBufferUploader);

    geoQuad->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(device,
                                                           cmdList, indices.data(), ibByteSize, geoQuad->IndexBufferUploader);

    geoQuad->VertexByteStride = sizeof(Vertex);
    geoQuad->VertexBufferByteSize = vbByteSize;
    geoQuad->IndexFormat = DXGI_FORMAT_R16_UINT;
    geoQuad->IndexBufferByteSize = ibByteSize;
    geoQuad->IndexCount = (UINT)indices.size();

    std::unique_ptr<Model> mQu = std::make_unique<Model>();

    mQu->meshes["quad"] = std::move(geoQuad);
    mModels["quad"] = std::move(mQu);


    /*grid*/
    vertices.clear();
    vertices.resize(grid.Vertices.size());
    indices.clear();
    indices.resize(grid.Indices32.size());

    vMin = XMLoadFloat3(&cMin);
    vMax = XMLoadFloat3(&cMax);

    for (size_t i = 0; i < grid.Vertices.size(); i++)
    {
        vertices[i].Pos = grid.Vertices[i].Position;
        vertices[i].Normal = grid.Vertices[i].Normal;
        vertices[i].TexC = grid.Vertices[i].TexC;
        vertices[i].TangentU = grid.Vertices[i].TangentU;

        XMVECTOR P = XMLoadFloat3(&vertices[i].Pos);

        vMin = XMVectorMin(vMin, P);
        vMax = XMVectorMax(vMax, P);
    }

    indices.insert(indices.end(), std::begin(grid.GetIndices16()), std::end(grid.GetIndices16()));

    vbByteSize = (UINT)vertices.size() * sizeof(Vertex);
    ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

    auto geoGrid = std::make_unique<Mesh>();
    geoGrid->name = "grid";

    ThrowIfFailed(D3DCreateBlob(vbByteSize, &geoGrid->VertexBufferCPU));
    CopyMemory(geoGrid->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

    ThrowIfFailed(D3DCreateBlob(ibByteSize, &geoGrid->IndexBufferCPU));
    CopyMemory(geoGrid->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

    geoGrid->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(device,
                                                            cmdList, vertices.data(), vbByteSize, geoGrid->VertexBufferUploader);

    geoGrid->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(device,
                                                           cmdList, indices.data(), ibByteSize, geoGrid->IndexBufferUploader);

    geoGrid->VertexByteStride = sizeof(Vertex);
    geoGrid->VertexBufferByteSize = vbByteSize;
    geoGrid->IndexFormat = DXGI_FORMAT_R16_UINT;
    geoGrid->IndexBufferByteSize = ibByteSize;
    geoGrid->IndexCount = (UINT)indices.size();

    std::unique_ptr<Model> mGr = std::make_unique<Model>();

    mGr->meshes["grid"] = std::move(geoGrid);
    mModels["grid"] = std::move(mGr);


    /*grid hitbox*/

    XMFLOAT3 v;
    XMStoreFloat3(&v, vMin);
    v.y = -0.025f;
    vMin = XMLoadFloat3(&v);

    XMStoreFloat3(&v, vMax);
    v.y = 0.025f;
    vMax = XMLoadFloat3(&v);

    XMStoreFloat3(&mModels["grid"]->boundingBox.Center, 0.5f * (vMin + vMax));
    XMStoreFloat3(&mModels["grid"]->boundingBox.Extents, 0.5f * (vMax - vMin));

    GeometryGenerator::MeshData boxMeshGrid = geoGen.CreateBox(mModels["grid"]->boundingBox.Extents.x * 2.f,
                                                               mModels["grid"]->boundingBox.Extents.y * 2.f,
                                                               mModels["grid"]->boundingBox.Extents.z * 2.f,
                                                               0);
    vertices.clear();
    vertices.resize(boxMeshGrid.Vertices.size());
    indices.clear();
    indices.resize(boxMeshGrid.Indices32.size());


    for (size_t i = 0; i < boxMeshGrid.Vertices.size(); i++)
    {
        XMStoreFloat3(&vertices[i].Pos, XMVectorAdd(XMLoadFloat3(&boxMeshGrid.Vertices[i].Position), XMLoadFloat3(&mModels["grid"]->boundingBox.Center)));
        vertices[i].Normal = boxMeshGrid.Vertices[i].Normal;
        vertices[i].TexC = boxMeshGrid.Vertices[i].TexC;
        vertices[i].TangentU = boxMeshGrid.Vertices[i].TangentU;
    }

    indices.insert(indices.end(), std::begin(boxMeshGrid.GetIndices16()), std::end(boxMeshGrid.GetIndices16()));

    vbByteSize = (UINT)vertices.size() * sizeof(Vertex);
    ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

    std::unique_ptr<Mesh> hitboxGrid = std::make_unique<Mesh>();

    hitboxGrid->name = "hitbox";
    hitboxGrid->dTexture = "default";
    hitboxGrid->dNormal = "default_nmap";
    hitboxGrid->IndexFormat = DXGI_FORMAT_R16_UINT;
    hitboxGrid->VertexByteStride = sizeof(Vertex);
    hitboxGrid->VertexBufferByteSize = vbByteSize;
    hitboxGrid->IndexBufferByteSize = ibByteSize;
    hitboxGrid->IndexCount = (UINT)indices.size();

    ThrowIfFailed(D3DCreateBlob(vbByteSize, &hitboxGrid->VertexBufferCPU));
    CopyMemory(hitboxGrid->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

    ThrowIfFailed(D3DCreateBlob(ibByteSize, &hitboxGrid->IndexBufferCPU));
    CopyMemory(hitboxGrid->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

    hitboxGrid->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(device,
                                                               cmdList, vertices.data(), vbByteSize, hitboxGrid->VertexBufferUploader);

    hitboxGrid->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(device,
                                                              cmdList, indices.data(), ibByteSize, hitboxGrid->IndexBufferUploader);


    mModels["grid"]->boundingBoxMesh = std::move(hitboxGrid);


    /*sphere*/


    vertices.clear();
    vertices.resize(sphere.Vertices.size());
    indices.clear();
    indices.resize(sphere.Indices32.size());

    vMin = XMLoadFloat3(&cMin);
    vMax = XMLoadFloat3(&cMax);

    for (size_t i = 0; i < sphere.Vertices.size(); i++)
    {
        vertices[i].Pos = sphere.Vertices[i].Position;
        vertices[i].Normal = sphere.Vertices[i].Normal;
        vertices[i].TexC = sphere.Vertices[i].TexC;
        vertices[i].TangentU = sphere.Vertices[i].TangentU;

        XMVECTOR P = XMLoadFloat3(&vertices[i].Pos);

        vMin = XMVectorMin(vMin, P);
        vMax = XMVectorMax(vMax, P);
    }

    indices.insert(indices.end(), std::begin(sphere.GetIndices16()), std::end(sphere.GetIndices16()));

    vbByteSize = (UINT)vertices.size() * sizeof(Vertex);
    ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

    auto geoSphere = std::make_unique<Mesh>();
    geoSphere->name = "sphere";

    ThrowIfFailed(D3DCreateBlob(vbByteSize, &geoSphere->VertexBufferCPU));
    CopyMemory(geoSphere->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

    ThrowIfFailed(D3DCreateBlob(ibByteSize, &geoSphere->IndexBufferCPU));
    CopyMemory(geoSphere->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

    geoSphere->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(device,
                                                              cmdList, vertices.data(), vbByteSize, geoSphere->VertexBufferUploader);

    geoSphere->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(device,
                                                             cmdList, indices.data(), ibByteSize, geoSphere->IndexBufferUploader);

    geoSphere->VertexByteStride = sizeof(Vertex);
    geoSphere->VertexBufferByteSize = vbByteSize;
    geoSphere->IndexFormat = DXGI_FORMAT_R16_UINT;
    geoSphere->IndexBufferByteSize = ibByteSize;
    geoSphere->IndexCount = (UINT)indices.size();

    std::unique_ptr<Model> mSp = std::make_unique<Model>();

    mSp->meshes["sphere"] = std::move(geoSphere);
    mModels["sphere"] = std::move(mSp);

    /*sphere hitbox*/

    XMStoreFloat3(&mModels["sphere"]->boundingBox.Center, 0.5f * (vMin + vMax));
    XMStoreFloat3(&mModels["sphere"]->boundingBox.Extents, 0.5f * (vMax - vMin));

    GeometryGenerator::MeshData boxMeshSp = geoGen.CreateBox(mModels["sphere"]->boundingBox.Extents.x * 2.f,
                                                             mModels["sphere"]->boundingBox.Extents.y * 2.f,
                                                             mModels["sphere"]->boundingBox.Extents.z * 2.f,
                                                             0);
    vertices.clear();
    vertices.resize(boxMeshSp.Vertices.size());
    indices.clear();
    indices.resize(boxMeshSp.Indices32.size());


    for (size_t i = 0; i < boxMeshSp.Vertices.size(); i++)
    {
        XMStoreFloat3(&vertices[i].Pos, XMVectorAdd(XMLoadFloat3(&boxMeshSp.Vertices[i].Position), XMLoadFloat3(&mModels["sphere"]->boundingBox.Center)));
        vertices[i].Normal = boxMeshSp.Vertices[i].Normal;
        vertices[i].TexC = boxMeshSp.Vertices[i].TexC;
        vertices[i].TangentU = boxMeshSp.Vertices[i].TangentU;
    }

    indices.insert(indices.end(), std::begin(boxMeshSp.GetIndices16()), std::end(boxMeshSp.GetIndices16()));

    vbByteSize = (UINT)vertices.size() * sizeof(Vertex);
    ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

    std::unique_ptr<Mesh> hitboxSphere = std::make_unique<Mesh>();

    hitboxSphere->name = "hitbox";
    hitboxSphere->dTexture = "default";
    hitboxSphere->dNormal = "default_nmap";
    hitboxSphere->IndexFormat = DXGI_FORMAT_R16_UINT;
    hitboxSphere->VertexByteStride = sizeof(Vertex);
    hitboxSphere->VertexBufferByteSize = vbByteSize;
    hitboxSphere->IndexBufferByteSize = ibByteSize;
    hitboxSphere->IndexCount = (UINT)indices.size();

    ThrowIfFailed(D3DCreateBlob(vbByteSize, &hitboxSphere->VertexBufferCPU));
    CopyMemory(hitboxSphere->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

    ThrowIfFailed(D3DCreateBlob(ibByteSize, &hitboxSphere->IndexBufferCPU));
    CopyMemory(hitboxSphere->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

    hitboxSphere->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(device,
                                                                 cmdList, vertices.data(), vbByteSize, hitboxSphere->VertexBufferUploader);

    hitboxSphere->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(device,
                                                                cmdList, indices.data(), ibByteSize, hitboxSphere->IndexBufferUploader);


    mModels["sphere"]->boundingBoxMesh = std::move(hitboxSphere);

    /*cylinder*/


    vertices.clear();
    vertices.resize(cylinder.Vertices.size());
    indices.clear();
    indices.resize(cylinder.Indices32.size());

    vMin = XMLoadFloat3(&cMin);
    vMax = XMLoadFloat3(&cMax);

    for (size_t i = 0; i < cylinder.Vertices.size(); i++)
    {
        vertices[i].Pos = cylinder.Vertices[i].Position;
        vertices[i].Normal = cylinder.Vertices[i].Normal;
        vertices[i].TexC = cylinder.Vertices[i].TexC;
        vertices[i].TangentU = cylinder.Vertices[i].TangentU;

        XMVECTOR P = XMLoadFloat3(&vertices[i].Pos);

        vMin = XMVectorMin(vMin, P);
        vMax = XMVectorMax(vMax, P);
    }

    indices.insert(indices.end(), std::begin(cylinder.GetIndices16()), std::end(cylinder.GetIndices16()));

    vbByteSize = (UINT)vertices.size() * sizeof(Vertex);
    ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

    auto geoCyl = std::make_unique<Mesh>();
    geoCyl->name = "cylinder";

    ThrowIfFailed(D3DCreateBlob(vbByteSize, &geoCyl->VertexBufferCPU));
    CopyMemory(geoCyl->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

    ThrowIfFailed(D3DCreateBlob(ibByteSize, &geoCyl->IndexBufferCPU));
    CopyMemory(geoCyl->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

    geoCyl->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(device,
                                                           cmdList, vertices.data(), vbByteSize, geoCyl->VertexBufferUploader);

    geoCyl->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(device,
                                                          cmdList, indices.data(), ibByteSize, geoCyl->IndexBufferUploader);

    geoCyl->VertexByteStride = sizeof(Vertex);
    geoCyl->VertexBufferByteSize = vbByteSize;
    geoCyl->IndexFormat = DXGI_FORMAT_R16_UINT;
    geoCyl->IndexBufferByteSize = ibByteSize;
    geoCyl->IndexCount = (UINT)indices.size();

    std::unique_ptr<Model> mCyl = std::make_unique<Model>();

    mCyl->meshes["cylinder"] = std::move(geoCyl);
    mModels["cylinder"] = std::move(mCyl);


    /*cylinder hitbox*/

    XMStoreFloat3(&mModels["cylinder"]->boundingBox.Center, 0.5f * (vMin + vMax));
    XMStoreFloat3(&mModels["cylinder"]->boundingBox.Extents, 0.5f * (vMax - vMin));

    GeometryGenerator::MeshData boxMeshCyl = geoGen.CreateBox(mModels["cylinder"]->boundingBox.Extents.x * 2.f,
                                                              mModels["cylinder"]->boundingBox.Extents.y * 2.f,
                                                              mModels["cylinder"]->boundingBox.Extents.z * 2.f,
                                                              0);
    vertices.clear();
    vertices.resize(boxMeshCyl.Vertices.size());
    indices.clear();
    indices.resize(boxMeshCyl.Indices32.size());


    for (size_t i = 0; i < boxMeshCyl.Vertices.size(); i++)
    {
        XMStoreFloat3(&vertices[i].Pos, XMVectorAdd(XMLoadFloat3(&boxMeshCyl.Vertices[i].Position), XMLoadFloat3(&mModels["cylinder"]->boundingBox.Center)));
        vertices[i].Normal = boxMeshCyl.Vertices[i].Normal;
        vertices[i].TexC = boxMeshCyl.Vertices[i].TexC;
        vertices[i].TangentU = boxMeshCyl.Vertices[i].TangentU;
    }

    indices.insert(indices.end(), std::begin(boxMeshCyl.GetIndices16()), std::end(boxMeshCyl.GetIndices16()));

    vbByteSize = (UINT)vertices.size() * sizeof(Vertex);
    ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

    std::unique_ptr<Mesh> hitboxCyl = std::make_unique<Mesh>();

    hitboxCyl->name = "hitbox";
    hitboxCyl->dTexture = "default";
    hitboxCyl->dNormal = "default_nmap";
    hitboxCyl->IndexFormat = DXGI_FORMAT_R16_UINT;
    hitboxCyl->VertexByteStride = sizeof(Vertex);
    hitboxCyl->VertexBufferByteSize = vbByteSize;
    hitboxCyl->IndexBufferByteSize = ibByteSize;
    hitboxCyl->IndexCount = (UINT)indices.size();

    ThrowIfFailed(D3DCreateBlob(vbByteSize, &hitboxCyl->VertexBufferCPU));
    CopyMemory(hitboxCyl->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

    ThrowIfFailed(D3DCreateBlob(ibByteSize, &hitboxCyl->IndexBufferCPU));
    CopyMemory(hitboxCyl->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

    hitboxCyl->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(device,
                                                              cmdList, vertices.data(), vbByteSize, hitboxCyl->VertexBufferUploader);

    hitboxCyl->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(device,
                                                             cmdList, indices.data(), ibByteSize, hitboxCyl->IndexBufferUploader);


    mModels["cylinder"]->boundingBoxMesh = std::move(hitboxCyl);
}