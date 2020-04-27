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
        L"data\\texture\\hud\\gui\\edit\\legend_full.png",
        L"data\\texture\\hud\\gui\\edit\\tex_win.png",
        L"data\\texture\\hud\\gui\\edit\\object.png"
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

    mTexturePreview = initHUDElement(Descriptors::TOOL_WIN, { 0.5f,0.5f }, 1.0f);

    mTexturePreview->TextureSize.x = 2048;
    mTexturePreview->TextureSize.y = 2048;
    mTexturePreview->Origin.x = float(mTexturePreview->TextureSize.x / 2);
    mTexturePreview->Origin.y = float(mTexturePreview->TextureSize.y / 2);
    mTexturePreview->SourceRectangle.left = 0;
    mTexturePreview->SourceRectangle.right = mTexturePreview->TextureSize.x;
    mTexturePreview->SourceRectangle.top = 0;
    mTexturePreview->SourceRectangle.bottom = mTexturePreview->TextureSize.y;

    mTexturePreview->NormalizedPosition = {0.93f,0.356f};
    mTexturePreview->ResolutionScale = resolutionScaleFactor * 0.06f;

    /*tool select window 0 - 3*/
    mHUDElements.push_back(initHUDElement(Descriptors::TOOL_WIN, { 0.93f,0.13f }));
    mHUDElements.push_back(initHUDElement(Descriptors::BUTTON_RSHOULDER, { 0.915f,0.205f }));
    mHUDElements.push_back(initHUDElement(Descriptors::RIGHT, { 0.96f,0.205f }, 0.6f));
    mHUDElements.push_back(initHUDElement(Descriptors::HEIGHT, { 0.93f,0.108f }, 1.25f));

    /*save window 4 - 5*/
    mHUDElements.push_back(initHUDElement(Descriptors::SAVE_WIN, saveWindowPos));
    mHUDElements.push_back(initHUDElement(Descriptors::SAVED, saveWindowIconPos, 0.8f));

    /*select window 6 - 12*/
    mHUDElements.push_back(initHUDElement(Descriptors::SELECT_WIN, { 0.125f, 0.85f }));
    mHUDElements.push_back(initHUDElement(Descriptors::SLIDER_FG_BLUE, { 0.125f, 0.805f }));
    mHUDElements.push_back(initHUDElement(Descriptors::SLIDER_FG_GREEN, { 0.125f, 0.875f }));
    mHUDElements.push_back(initHUDElement(Descriptors::SLIDER_FG_RED, { 0.125f, 0.9425f }));
    mHUDElements.push_back(initHUDElement(Descriptors::SLIDER_BLUE, { 0.046f, 0.805f }, 0.85f));
    mHUDElements.push_back(initHUDElement(Descriptors::SLIDER_GREEN, { 0.046f, 0.875f }, 0.85f));
    mHUDElements.push_back(initHUDElement(Descriptors::SLIDER_RED, { 0.046f, 0.9425f }, 0.85f));

    /*legend window 13 - 14*/
    mHUDElements.push_back(initHUDElement(Descriptors::LEGEND_WIN, { 0.08f, 0.4f }));
    mHUDElements.push_back(initHUDElement(Descriptors::LEGEND_FULL_WIN, { -0.08f, 0.4f }));

    /*texture win 15 - 17*/
    mHUDElements.push_back(initHUDElement(Descriptors::TEXTURE_WIN, { 0.93f, 0.38f }));
    mHUDElements.push_back(initHUDElement(Descriptors::BUTTON_Y, { 0.915f,0.455f }, 0.7f));
    mHUDElements.push_back(initHUDElement(Descriptors::RIGHT, { 0.96f,0.455f }, 0.6f));

    /*set visibility*/
    for (int i = 6; i < 15; i++) mHUDElements[i]->hudVisibility = HUDVisibility::NOT_OBJECT;
    mHUDElements[15]->hudVisibility = HUDVisibility::PAINT;
    mHUDElements[16]->hudVisibility = HUDVisibility::PAINT;
    mHUDElements[17]->hudVisibility = HUDVisibility::PAINT;

}

void EditModeHUD::update()
{
    auto dispSetting = ServiceProvider::getSettings()->displaySettings;
    auto editSetting = ServiceProvider::getEditSettings();

    /*show correct tool*/
    mHUDElements[3]->TexDescriptor = editSetting->toolMode == EditTool::Height ? Descriptors::HEIGHT : editSetting->toolMode == EditTool::Paint ? Descriptors::PAINT : Descriptors::OBJECT;

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
        mHUDElements[13]->NormalizedPosition.x = (editSetting->legendStatus ? 1 : -1) * MathHelper::lerpH(-legendWindowPos.x,
                                                                   legendWindowPos.x,
                                                                   editSetting->legendAnim / editSetting->legenAnimDur);

        mHUDElements[14]->NormalizedPosition.x = (editSetting->legendStatus ? -1 : 1) * MathHelper::lerpH(-legendWindowPos.x,
                                                                   legendWindowPos.x,
                                                                   editSetting->legendAnim / editSetting->legenAnimDur);
    }


    /*radius slider*/
    mHUDElements[10]->NormalizedPosition.x = MathHelper::mapH(editSetting->BaseRadius,
                                                           1.35f,
                                                           editSetting->BaseSelectSize,
                                                           sliderMinMax.x,
                                                           sliderMinMax.y);

    mHUDElements[7]->SourceRectangle.right = (LONG)MathHelper::mapH(editSetting->BaseRadius,
                                                              1.35f,
                                                              editSetting->BaseSelectSize,
                                                              0.0f,
                                                              (float)mHUDElements[7]->TextureSize.x);

    /*falloff slider*/
    mHUDElements[11]->NormalizedPosition.x = MathHelper::mapH(editSetting->FallOffRatio,
                                                             editSetting->fallOffRatioMin,
                                                             editSetting->fallOffRatioMax,
                                                             sliderMinMax.x,
                                                             sliderMinMax.y);

    mHUDElements[8]->SourceRectangle.right = (LONG)MathHelper::mapH(editSetting->FallOffRatio,
                                                              editSetting->fallOffRatioMin,
                                                              editSetting->fallOffRatioMax,
                                                              0.0f,
                                                              (float)mHUDElements[7]->TextureSize.x);

    /*fill slider*/

    if (editSetting->toolMode == EditTool::Height)
    {
        mHUDElements[12]->NormalizedPosition.x = MathHelper::mapH(editSetting->heightIncrease,
                                                                 editSetting->heightIncreaseMin,
                                                                 editSetting->heightIncreaseMax,
                                                                 sliderMinMax.x,
                                                                 sliderMinMax.y);

        mHUDElements[9]->SourceRectangle.right = (LONG)MathHelper::mapH(editSetting->heightIncrease,
                                                                  editSetting->heightIncreaseMin,
                                                                  editSetting->heightIncreaseMax,
                                                                  0.0f,
                                                                  (float)mHUDElements[9]->TextureSize.x);

    }
    else if (editSetting->toolMode == EditTool::Paint)
    {
        mHUDElements[12]->NormalizedPosition.x = MathHelper::mapH(editSetting->paintIncrease,
                                                                 editSetting->paintIncreaseMin,
                                                                 editSetting->paintIncreaseMax,
                                                                 sliderMinMax.x,
                                                                 sliderMinMax.y);

        mHUDElements[9]->SourceRectangle.right = (LONG)MathHelper::mapH(editSetting->paintIncrease,
                                                                  editSetting->paintIncreaseMin,
                                                                  editSetting->paintIncreaseMax,
                                                                  0.0f,
                                                                  (float)mHUDElements[9]->TextureSize.x);
    }

    for (auto& e : mHUDElements)
    {
        /*recalculate actual screen position*/

        e->ScreenPosition.x = e->NormalizedPosition.x * dispSetting.ResolutionWidth;
        e->ScreenPosition.y = e->NormalizedPosition.y * dispSetting.ResolutionHeight;

    }

    mTexturePreview->ScreenPosition.x = mTexturePreview->NormalizedPosition.x * dispSetting.ResolutionWidth;
    mTexturePreview->ScreenPosition.y = mTexturePreview->NormalizedPosition.y * dispSetting.ResolutionHeight;

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

        if (e->hudVisibility == HUDVisibility::NOT_OBJECT && ServiceProvider::getEditSettings()->toolMode == EditTool::Object) continue;
        if (e->hudVisibility == HUDVisibility::HEIGHT && ServiceProvider::getEditSettings()->toolMode != EditTool::Height)continue;
        if (e->hudVisibility == HUDVisibility::PAINT && ServiceProvider::getEditSettings()->toolMode != EditTool::Paint)continue;
        if (e->hudVisibility == HUDVisibility::OBJECT && ServiceProvider::getEditSettings()->toolMode != EditTool::Object)continue;

        mSpriteBatch->Draw(m_resourceDescriptors->GetGpuHandle(e->TexDescriptor),
                            e->TextureSize,
                            e->ScreenPosition, 
                            &e->SourceRectangle, 
                            Colors::White, 
                            e->Rotation,
                            e->Origin,
                            e->ResolutionScale * e->Scale);
    }



    mSpriteBatch->End();

    /*draw texture of paint mode*/
    if (ServiceProvider::getEditSettings()->toolMode == EditTool::Paint)
    {

        ID3D12DescriptorHeap* descriptorHeaps[] = { renderResource->mSrvDescriptorHeap.Get() };
        renderResource->cmdList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);


        mSpriteBatch->Begin(renderResource->cmdList);

        mSpriteBatch->Draw(ServiceProvider::getActiveLevel()->mTerrain->blendTexturesHandle[ServiceProvider::getEditSettings()->usedTextureIndex],
                           mTexturePreview->TextureSize,
                           mTexturePreview->ScreenPosition,
                           &mTexturePreview->SourceRectangle,
                           Colors::White,
                           mTexturePreview->Rotation,
                           mTexturePreview->Origin,
                           mTexturePreview->ResolutionScale * mTexturePreview->Scale);

        

        mSpriteBatch->End();
    }

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

    element->SourceRectangle.left = 0;
    element->SourceRectangle.right = element->TextureSize.x;
    element->SourceRectangle.top = 0;
    element->SourceRectangle.bottom = element->TextureSize.y;

    return std::move(element);
}
