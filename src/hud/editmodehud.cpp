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

    mGraphicsMemory = std::make_unique<GraphicsMemory>(renderResource->device);

    m_resourceDescriptors = std::make_unique<DescriptorHeap>(renderResource->device,
                                                             D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
                                                             D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
                                                             (int)Descriptors::Count);

    ResourceUploadBatch resourceUpload(renderResource->device);

    resourceUpload.Begin();

    /*load png textures*/
    mTextures.resize(Descriptors::Count);

    std::vector<std::wstring> texPaths = {
        L"data\\texture\\hud\\button\\a.png",
        L"data\\texture\\hud\\button\\b.png",
        L"data\\texture\\hud\\button\\x.png",
        L"data\\texture\\hud\\button\\y.png",
        L"data\\texture\\hud\\button\\l_b.png",
        L"data\\texture\\hud\\button\\r_b.png",
        L"data\\texture\\hud\\button\\l_t.png",
        L"data\\texture\\hud\\button\\r_t.png",
        L"data\\texture\\hud\\button\\windows.png",
        L"data\\texture\\hud\\button\\menu.png",
        L"data\\texture\\hud\\button\\dpad.png",
        L"data\\texture\\hud\\button\\dpad_down.png",
        L"data\\texture\\hud\\button\\dpad_up.png",
        L"data\\texture\\hud\\button\\dpad_left.png",
        L"data\\texture\\hud\\button\\dpad_right.png",
        L"data\\texture\\hud\\button\\l_stick.png",
        L"data\\texture\\hud\\button\\r_stick.png",
    };

    ASSERT(texPaths.size() == Descriptors::Count);

    for (int i = 0; i < mTextures.size(); i++)
    {
        CreateWICTextureFromFile(renderResource->device, resourceUpload, texPaths[i].c_str(),
                                 mTextures[i].ReleaseAndGetAddressOf());


        CreateShaderResourceView(renderResource->device, mTextures[i].Get(),
                                 m_resourceDescriptors->GetCpuHandle(i));
    }

    RenderTargetState rtState(DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_FORMAT_D32_FLOAT);

    SpriteBatchPipelineStateDescription pd(rtState, &CommonStates::NonPremultiplied);
    mSpriteBatch = std::make_unique<SpriteBatch>(renderResource->device, resourceUpload, pd);


    auto uploadResourcesFinished = resourceUpload.End(renderResource->cmdQueue);

    uploadResourcesFinished.wait();

    /*set viewport of spritebatch*/
    D3D12_VIEWPORT viewport = { 0.0f, 0.0f,
    static_cast<float>(ServiceProvider::getSettings()->displaySettings.ResolutionWidth),
    static_cast<float>(ServiceProvider::getSettings()->displaySettings.ResolutionHeight),
    D3D12_MIN_DEPTH, D3D12_MAX_DEPTH };
    mSpriteBatch->SetViewport(viewport);

    float scaleFactor = ServiceProvider::getSettings()->displaySettings.ResolutionWidth / DEFAULT_WIDTH;

    /*create HUD Elements for the GUI*/

    auto hE = std::make_unique<HUDElement>(Descriptors::BUTTON_B);

    hE->TextureSize = GetTextureSize(mTextures[hE->TexDescriptor].Get());
    hE->Origin.x = float(hE->TextureSize.x / 2);
    hE->Origin.y = float(hE->TextureSize.y / 2);

    hE->ScreenPosition.x = 0;
    hE->ScreenPosition.y = 0;
    hE->ResolutionScale = scaleFactor;

    mHUDElements.push_back(std::move(hE));

}

void EditModeHUD::update()
{
}

void EditModeHUD::draw()
{
    auto renderResource = ServiceProvider::getRenderResource();
    ID3D12DescriptorHeap* heaps[] = { m_resourceDescriptors->Heap() };
    renderResource->cmdList->SetDescriptorHeaps(_countof(heaps), heaps);

    mSpriteBatch->Begin(renderResource->cmdList);

    for (const auto& e : mHUDElements)
    {
        if (!e->Visible) continue;

        mSpriteBatch->Draw(m_resourceDescriptors->GetGpuHandle(e->TexDescriptor),
                            e->TextureSize,
                            e->ScreenPosition, 
                            nullptr, 
                            Colors::White, 
                            e->Rotation,
                            e->Origin,
                            e->ResolutionScale * e->Scale);
    }



    mSpriteBatch->End();

}

void EditModeHUD::commit()
{
    mGraphicsMemory->Commit(ServiceProvider::getRenderResource()->cmdQueue);
}
