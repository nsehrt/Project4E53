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
        /*xbox buttons*/
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
        /*GUI Elements*/
        L"data\\texture\\hud\\gui\\edit\\tool_win.png",
        L"data\\texture\\hud\\gui\\edit\\height.png",
        L"data\\texture\\hud\\gui\\edit\\paint.png",
        L"data\\texture\\hud\\gui\\edit\\left.png",
        L"data\\texture\\hud\\gui\\edit\\right.png",
        L"data\\texture\\hud\\gui\\edit\\save_win.png",
        L"data\\texture\\hud\\gui\\edit\\save.png",
        L"data\\texture\\hud\\gui\\edit\\failed.png",
        L"data\\texture\\hud\\gui\\edit\\select_win.png",
        L"data\\texture\\hud\\gui\\edit\\slider_fg_blue.png",
        L"data\\texture\\hud\\gui\\edit\\slider_fg_green.png",
        L"data\\texture\\hud\\gui\\edit\\slider_fg_red.png",
        L"data\\texture\\hud\\gui\\edit\\slider_blue.png",
        L"data\\texture\\hud\\gui\\edit\\slider_green.png",
        L"data\\texture\\hud\\gui\\edit\\slider_red.png",
        L"data\\texture\\hud\\gui\\edit\\legend.png",
        L"data\\texture\\hud\\gui\\edit\\legend_full.png"
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

    resolutionScaleFactor = ServiceProvider::getSettings()->displaySettings.ResolutionWidth / DEFAULT_WIDTH;

    /*create HUD Elements for the GUI*/

    /*tool select window 0 - 3*/
    mHUDElements.push_back(initHUDElement(Descriptors::TOOL_WIN, { 0.93f,0.13f }));
    mHUDElements.push_back(initHUDElement(Descriptors::BUTTON_RSHOULDER, { 0.915f,0.205f }));
    mHUDElements.push_back(initHUDElement(Descriptors::RIGHT, { 0.96f,0.205f }, 0.6f));
    mHUDElements.push_back(initHUDElement(Descriptors::HEIGHT, { 0.93f,0.108f }, 1.25f));

    /*save window 4 - 5*/
    mHUDElements.push_back(initHUDElement(Descriptors::SAVE_WIN, saveWindowPos));
    mHUDElements.push_back(initHUDElement(Descriptors::SAVED, saveWindowIconPos, 0.8f));

    /*select window 6 - 9*/
    mHUDElements.push_back(initHUDElement(Descriptors::SELECT_WIN, { 0.125f, 0.85f }));
    mHUDElements.push_back(initHUDElement(Descriptors::SLIDER_BLUE, { 0.046f, 0.805f }, 0.85f));
    mHUDElements.push_back(initHUDElement(Descriptors::SLIDER_GREEN, { 0.046f, 0.875f }, 0.85f));
    mHUDElements.push_back(initHUDElement(Descriptors::SLIDER_RED, { 0.046f, 0.9425f }, 0.85f));

    /*legend window 10 - 11*/
    mHUDElements.push_back(initHUDElement(Descriptors::LEGEND_WIN, { 0.08f, 0.4f }));
    mHUDElements.push_back(initHUDElement(Descriptors::LEGEND_FULL_WIN, { -0.08f, 0.4f }));

}

void EditModeHUD::update()
{
    auto dispSetting = ServiceProvider::getSettings()->displaySettings;
    auto editSetting = ServiceProvider::getEditSettings();

    /*show correct tool*/
    mHUDElements[3]->TexDescriptor = editSetting->toolMode == EditTool::Height ? Descriptors::HEIGHT : Descriptors::PAINT;

    /*save window*/
    mHUDElements[5]->TexDescriptor = editSetting->saveSuccess ? Descriptors::SAVED : Descriptors::FAILED;

    if (editSetting->savedAnim > 0.0f)
    {
        if (editSetting->savedAnim > saveWindowLinger)
        {
            mHUDElements[4]->NormalizedPosition.y = MathHelper::lerpH(-saveWindowPos.y,
                                                         saveWindowPos.y,
                                                         1.0f - (editSetting->savedAnim - saveWindowLinger));

            mHUDElements[5]->NormalizedPosition.y = mHUDElements[4]->NormalizedPosition.y;
        }
    }
    else
    {
        mHUDElements[4]->NormalizedPosition.y = -saveWindowIconPos.y;
        mHUDElements[5]->NormalizedPosition.y = -saveWindowIconPos.y;
    }

    /*legend window*/
    if (editSetting->legendAnim > 0.0f)
    {
        mHUDElements[10]->NormalizedPosition.x = (editSetting->legendStatus ? 1 : -1) * MathHelper::lerpH(-legendWindowPos.x,
                                                                   legendWindowPos.x,
                                                                   editSetting->legendAnim / editSetting->legenAnimDur);

        mHUDElements[11]->NormalizedPosition.x = (editSetting->legendStatus ? -1 : 1) * MathHelper::lerpH(-legendWindowPos.x,
                                                                   legendWindowPos.x,
                                                                   editSetting->legendAnim / editSetting->legenAnimDur);
    }


    /*radius slider*/
    mHUDElements[7]->NormalizedPosition.x = MathHelper::mapH(editSetting->BaseRadius,
                                                           1.35f,
                                                           editSetting->BaseSelectSize,
                                                           sliderMinMax.x,
                                                           sliderMinMax.y);

    /*falloff slider*/
    mHUDElements[8]->NormalizedPosition.x = MathHelper::mapH(editSetting->FallOffRatio,
                                                             editSetting->fallOffRatioMin,
                                                             editSetting->fallOffRatioMax,
                                                             sliderMinMax.x,
                                                             sliderMinMax.y);

    /*fill slider*/

    if (editSetting->toolMode == EditTool::Height)
    {
        mHUDElements[9]->NormalizedPosition.x = MathHelper::mapH(editSetting->heightIncrease,
                                                                 editSetting->heightIncreaseMin,
                                                                 editSetting->heightIncreaseMax,
                                                                 sliderMinMax.x,
                                                                 sliderMinMax.y);
    }
    else if (editSetting->toolMode == EditTool::Paint)
    {
        mHUDElements[9]->NormalizedPosition.x = MathHelper::mapH(editSetting->paintIncrease,
                                                                 editSetting->paintIncreaseMin,
                                                                 editSetting->paintIncreaseMax,
                                                                 sliderMinMax.x,
                                                                 sliderMinMax.y);
    }



    for (auto& e : mHUDElements)
    {
        /*recalculate actual screen position*/

        e->ScreenPosition.x = e->NormalizedPosition.x * dispSetting.ResolutionWidth;
        e->ScreenPosition.y = e->NormalizedPosition.y * dispSetting.ResolutionHeight;

    }

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

std::unique_ptr<EditModeHUD::HUDElement> EditModeHUD::initHUDElement(Descriptors desc, DirectX::SimpleMath::Vector2 nPos, float scaleF)
{
    auto element = std::make_unique<HUDElement>(desc);

    element->TextureSize = GetTextureSize(mTextures[element->TexDescriptor].Get());
    element->Origin.x = float(element->TextureSize.x / 2);
    element->Origin.y = float(element->TextureSize.y / 2);

    element->NormalizedPosition = nPos;
    element->ResolutionScale = resolutionScaleFactor * scaleF;

    return std::move(element);
}
