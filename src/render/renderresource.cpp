#include "renderresource.h"
#include "../util/serviceprovider.h"

void RenderResource::init(ID3D12Device* _device, ID3D12GraphicsCommandList* _cmdList, const std::filesystem::path& _texturePath, const std::filesystem::path& _modelPath)
{
    device = _device;
    cmdList = _cmdList;

    int textureCounter = 0;
    int modelCounter = 0;

    /*load all textures*/
    for (const auto& entry : std::filesystem::recursive_directory_iterator(_texturePath))
    {
        if (loadTexture(entry.path().u8string()))
        {
            textureCounter++;
        }
        else
        {
            std::stringstream str;
            str << "Failed to load texture " << entry.path().u8string() << "!";
            ServiceProvider::getVSLogger()->print<Severity::Warning>(str.str().c_str());
        }
    }

    std::stringstream str;
    str << "Successfully loaded " << textureCounter << " textures.";
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

    std::stringstream str;
    str << "Successfully loaded " << modelCounter << " models.";
    ServiceProvider::getVSLogger()->print<Severity::Info>(str.str().c_str());

}

bool RenderResource::loadTexture(const std::string& file)
{
    auto texMap = std::make_unique<Texture>();
    texMap->Name = file;
    texMap->Filename = AnsiToWString(file);

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
    return false;
}

bool RenderResource::buildDescriptorHeap()
{
    return false;
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
        ServiceProvider::getSettings()->graphicSettings.AnisotropicFiltering);                               // maxAnisotropy

    const CD3DX12_STATIC_SAMPLER_DESC anisotropicClamp(
        5, // shaderRegister
        D3D12_FILTER_ANISOTROPIC, // filter
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressW
        0.0f,                              // mipLODBias
        ServiceProvider::getSettings()->graphicSettings.AnisotropicFiltering);                                // maxAnisotropy

    return {
        pointWrap, pointClamp,
        linearWrap, linearClamp,
        anisotropicWrap, anisotropicClamp };
}