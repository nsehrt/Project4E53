#include "editmodehud.h"
#include "ResourceUploadBatch.h"
#include "WICTextureLoader.h"
#include "DirectXHelpers.h"
#include "CommonStates.h"
#include "../util/serviceprovider.h"

EditModeHUD::EditModeHUD()
{
}

void EditModeHUD::init()
{
    auto renderResource = ServiceProvider::getRenderResource();

    m_graphicsMemory = std::make_unique<GraphicsMemory>(renderResource->device);

    m_resourceDescriptors = std::make_unique<DescriptorHeap>(renderResource->device,
                                                             D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
                                                             D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
                                                             1);

    ResourceUploadBatch resourceUpload(renderResource->device);

    resourceUpload.Begin();

        CreateWICTextureFromFile(renderResource->device, resourceUpload, L"data\\texture\\hud\\button\\a.png",
        m_texture.ReleaseAndGetAddressOf());

    CreateShaderResourceView(renderResource->device, m_texture.Get(),
                             m_resourceDescriptors->GetCpuHandle(Descriptors::Cat));

    RenderTargetState rtState(DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_FORMAT_D32_FLOAT);

    SpriteBatchPipelineStateDescription pd(rtState, &CommonStates::NonPremultiplied);
    m_spriteBatch = std::make_unique<SpriteBatch>(renderResource->device, resourceUpload, pd);


    auto uploadResourcesFinished = resourceUpload.End(renderResource->cmdQueue);

    uploadResourcesFinished.wait();


    XMUINT2 catSize = GetTextureSize(m_texture.Get());

    m_origin.x = float(catSize.x / 2);
    m_origin.y = float(catSize.y / 2);


    D3D12_VIEWPORT viewport = { 0.0f, 0.0f,
    static_cast<float>(1600), static_cast<float>(900),
    D3D12_MIN_DEPTH, D3D12_MAX_DEPTH };
    m_spriteBatch->SetViewport(viewport);

    m_screenPos.x = 1600 / 2.f;
    m_screenPos.y = 1200 / 2.f;
}

void EditModeHUD::update()
{
}

void EditModeHUD::draw()
{
    auto renderResource = ServiceProvider::getRenderResource();
    ID3D12DescriptorHeap* heaps[] = { m_resourceDescriptors->Heap() };
    renderResource->cmdList->SetDescriptorHeaps(_countof(heaps), heaps);

    m_spriteBatch->Begin(renderResource->cmdList);

    m_spriteBatch->Draw(m_resourceDescriptors->GetGpuHandle(Descriptors::Cat),
                        GetTextureSize(m_texture.Get()),
                        m_screenPos, nullptr, Colors::White, 0.f, m_origin);

    m_spriteBatch->End();

}
