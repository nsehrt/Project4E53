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
                                                             (int)TextureDescriptors::TCount + (int)FontDescriptors::FCount);

    ResourceUploadBatch resourceUpload(renderResource->device);

    resourceUpload.Begin();

    /*load png textures*/
    mTextures.resize(TextureDescriptors::TCount);
    mFonts.resize(FontDescriptors::FCount);

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
        L"data\\texture\\hud\\gui\\edit\\tex_win.png",
        L"data\\texture\\hud\\gui\\edit\\object.png",
        L"data\\texture\\hud\\gui\\edit\\tool_axis_select.png",
        L"data\\texture\\hud\\gui\\edit\\tool_axis_select_cursor.png",
        L"data\\texture\\hud\\gui\\edit\\object_info.png",
        L"data\\texture\\hud\\gui\\edit\\camera.png",
        L"data\\texture\\hud\\gui\\edit\\meta.png",
        L"data\\texture\\hud\\gui\\edit\\model_prop.png",
        L"data\\texture\\hud\\gui\\edit\\success.png",
        L"data\\texture\\hud\\gui\\edit\\crosshair.png",
        L"data\\texture\\hud\\gui\\edit\\light.png",
        L"data\\texture\\hud\\gui\\edit\\light_info.png"
    };

    std::vector<std::wstring> fontPaths = {
         L"data\\font\\editor64.font"
    };

    ASSERT(texPaths.size() == TextureDescriptors::TCount);
    ASSERT(fontPaths.size() == FontDescriptors::FCount);

    /*load textures*/
    for (int i = 0; i < mTextures.size(); i++)
    {
        CreateWICTextureFromFile(renderResource->device, resourceUpload, texPaths[i].c_str(),
                                 mTextures[i].ReleaseAndGetAddressOf());

        CreateShaderResourceView(renderResource->device, mTextures[i].Get(),
                                 m_resourceDescriptors->GetCpuHandle(i));
    }

    /*load fonts*/
    for (int i = 0; i < mFonts.size(); i++)
    {
        mFonts[i] = std::make_unique<SpriteFont>(renderResource->device, resourceUpload, fontPaths[i].c_str(),
                                                 m_resourceDescriptors->GetCpuHandle(i + TextureDescriptors::TCount),
                                                 m_resourceDescriptors->GetGpuHandle(i + TextureDescriptors::TCount));
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

    mTexturePreview = initHUDElement(TextureDescriptors::TOOL_WIN, { 0.5f,0.5f }, 1.0f);

    mTexturePreview->TextureSize.x = 2048;
    mTexturePreview->TextureSize.y = 2048;
    mTexturePreview->Origin.x = float(mTexturePreview->TextureSize.x / 2);
    mTexturePreview->Origin.y = float(mTexturePreview->TextureSize.y / 2);
    mTexturePreview->SourceRectangle.left = 0;
    mTexturePreview->SourceRectangle.right = mTexturePreview->TextureSize.x;
    mTexturePreview->SourceRectangle.top = 0;
    mTexturePreview->SourceRectangle.bottom = mTexturePreview->TextureSize.y;

    mTexturePreview->NormalizedPosition = { 0.93f,0.356f };
    mTexturePreview->ResolutionScale = resolutionScaleFactor * 0.06f;

    /*tool select window 0 - 3*/
    mHUDElements.push_back(initHUDElement(TextureDescriptors::TOOL_WIN, { 0.93f,0.13f }));
    mHUDElements.push_back(initHUDElement(TextureDescriptors::BUTTON_LSHOULDER, { 0.905f,0.205f }));
    mHUDElements.push_back(initHUDElement(TextureDescriptors::BUTTON_RSHOULDER, { 0.955f,0.205f }));
    mHUDElements.push_back(initHUDElement(TextureDescriptors::HEIGHT, { 0.93f,0.108f }, 1.25f));

    /*save window 4 - 5*/
    mHUDElements.push_back(initHUDElement(TextureDescriptors::SAVE_WIN, saveWindowPos));
    mHUDElements.push_back(initHUDElement(TextureDescriptors::SAVED, saveWindowIconPos, 0.8f));

    /*select window 6 - 12*/
    mHUDElements.push_back(initHUDElement(TextureDescriptors::SELECT_WIN, { 0.88f, 0.85f }));
    mHUDElements.push_back(initHUDElement(TextureDescriptors::SLIDER_FG_BLUE, { 0.88f, 0.805f }));
    mHUDElements.push_back(initHUDElement(TextureDescriptors::SLIDER_FG_GREEN, { 0.88f, 0.875f }));
    mHUDElements.push_back(initHUDElement(TextureDescriptors::SLIDER_FG_RED, { 0.88f, 0.9425f }));
    mHUDElements.push_back(initHUDElement(TextureDescriptors::SLIDER_BLUE, { 0.046f, 0.805f }, 0.85f));
    mHUDElements.push_back(initHUDElement(TextureDescriptors::SLIDER_GREEN, { 0.046f, 0.875f }, 0.85f));
    mHUDElements.push_back(initHUDElement(TextureDescriptors::SLIDER_RED, { 0.046f, 0.9425f }, 0.85f));

    /*legend window 13 - 14*/
    mHUDElements.push_back(initHUDElement(TextureDescriptors::FAILED, { 0.08f, 0.4f }));
    mHUDElements.push_back(initHUDElement(TextureDescriptors::FAILED, { -0.08f, 0.4f }));

    /*texture win 15 - 17*/
    mHUDElements.push_back(initHUDElement(TextureDescriptors::TEXTURE_WIN, { 0.93f, 0.38f }));
    mHUDElements.push_back(initHUDElement(TextureDescriptors::BUTTON_Y, { 0.915f,0.455f }, 0.7f));
    mHUDElements.push_back(initHUDElement(TextureDescriptors::RIGHT, { 0.96f,0.455f }, 0.6f));

    /*hud for game object mode 18-23*/
    mHUDElements.push_back(initHUDElement(TextureDescriptors::TOOL_AXIS_WIN, { 0.905f, 0.825f }));
    mHUDElements.push_back(initHUDElement(TextureDescriptors::TOOL_AXIS_CURSOR, { 0.8525f, 0.734f }));
    mHUDElements.push_back(initHUDElement(TextureDescriptors::SLIDER_BLUE, { 0.837f, 0.7675f }, 0.5f));
    mHUDElements.push_back(initHUDElement(TextureDescriptors::SLIDER_GREEN, { 0.8885f,  0.7675f }, 0.5f));
    mHUDElements.push_back(initHUDElement(TextureDescriptors::SLIDER_RED, { 0.94f,  0.7675f }, 0.5f));

    mHUDElements.push_back(initHUDElement(TextureDescriptors::OBJECT_INFO_WIN, { 0.89f, 0.56f }));

    /*object meta mode 24-29*/
    mHUDElements.push_back(initHUDElement(TextureDescriptors::MODEL_PROP, { 0.905f, 0.825f }));
    mHUDElements.push_back(initHUDElement(TextureDescriptors::SLIDER_RED, { 0.83f, 0.8f }, 0.35f));

    mHUDElements.push_back(initHUDElement(TextureDescriptors::FAILED, { 0.9125f, 0.80f }, 0.3f));
    mHUDElements.push_back(initHUDElement(TextureDescriptors::FAILED, { 0.9125f, 0.825f }, 0.3f));
    mHUDElements.push_back(initHUDElement(TextureDescriptors::FAILED, { 0.9125f, 0.85f }, 0.3f));
    mHUDElements.push_back(initHUDElement(TextureDescriptors::FAILED, { 0.9125f, 0.875f }, 0.3f));

    /*camera crosshair 30*/
    mHUDElements.push_back(initHUDElement(TextureDescriptors::CROSSHAIR, { 0.5f, 0.5f }, 1.0f));

    /*light window 31*/
    mHUDElements.push_back(initHUDElement(TextureDescriptors::LIGHT_INFO_WIN, { 0.89f, 0.85f }, 1.0f));

    /*fonts 0-9*/
    mFontElements.push_back(initFontElement(FontDescriptors::Editor64, { 0.842f, 0.525f }, 0.2f));
    mFontElements.push_back(initFontElement(FontDescriptors::Editor64, { 0.842f, 0.56f }, 0.2f));
    mFontElements.push_back(initFontElement(FontDescriptors::Editor64, { 0.842f, 0.595f }, 0.2f));

    mFontElements.push_back(initFontElement(FontDescriptors::Editor64, { 0.888f,0.525f }, 0.2f));
    mFontElements.push_back(initFontElement(FontDescriptors::Editor64, { 0.888f,0.56f }, 0.2f));
    mFontElements.push_back(initFontElement(FontDescriptors::Editor64, { 0.888f,0.595f }, 0.2f));

    mFontElements.push_back(initFontElement(FontDescriptors::Editor64, { 0.935f,0.525f }, 0.2f));
    mFontElements.push_back(initFontElement(FontDescriptors::Editor64, { 0.935f,0.56f }, 0.2f));
    mFontElements.push_back(initFontElement(FontDescriptors::Editor64, { 0.935f,0.595f }, 0.2f));

    mFontElements.push_back(initFontElement(FontDescriptors::Editor64, { 0.795f,0.46f }, 0.3f));

    /*object meta font 10-12*/
    mFontElements.push_back(initFontElement(FontDescriptors::Editor64, { 0.83f,0.735f }, 0.25f));
    mFontElements.push_back(initFontElement(FontDescriptors::Editor64, { 0.83f,0.93f }, 0.2f));
    mFontElements.push_back(initFontElement(FontDescriptors::Editor64, { 0.83f,0.71f }, 0.25f));

    /*light info font 13 - 25*/

    mFontElements.push_back(initFontElement(FontDescriptors::Editor64, { 0.8f,0.725f }, 0.25f));


    for (UINT i = 0; i < 3; i++)
    {
        for (UINT j = 0; j < 3; j++)
        {
            mFontElements.push_back(initFontElement(FontDescriptors::Editor64, { 0.843f + 0.048f * j,0.79f + i * 0.03f }, 0.2f));
        }
    }

    mFontElements.push_back(initFontElement(FontDescriptors::Editor64, { 0.87f,0.905 }, 0.2f));
    mFontElements.push_back(initFontElement(FontDescriptors::Editor64, { 0.87f,0.93f }, 0.2f));

    /*set visibility*/
    for (int i = 6; i < 15; i++) mHUDElements[i]->hudVisibility = HUDVisibility::HEIGHT_AND_PAINT;
    mHUDElements[10]->Visible = false;
    mHUDElements[11]->Visible = false;
    mHUDElements[12]->Visible = false;
    mHUDElements[15]->hudVisibility = HUDVisibility::PAINT;
    mHUDElements[16]->hudVisibility = HUDVisibility::PAINT;
    mHUDElements[17]->hudVisibility = HUDVisibility::PAINT;

    mHUDElements[13]->Visible = false;
    mHUDElements[14]->Visible = false;

    for (int i = 18; i < 23; i++)
    {
        mHUDElements[i]->hudVisibility = HUDVisibility::OBJECT;
    }

    mHUDElements[23]->hudVisibility = HUDVisibility::BOTH_OBJECT;

    for (int i = 24; i < 30; i++)
    {
        mHUDElements[i]->hudVisibility = HUDVisibility::OBJECT_META;
    }
    mHUDElements[30]->hudVisibility = HUDVisibility::FPS_CAMERA;
    mHUDElements[31]->hudVisibility = HUDVisibility::LIGHT;

    for (int i = 0; i < mFontElements.size(); i++)
    {
        mFontElements[i]->hudVisibility = HUDVisibility::BOTH_OBJECT;
    }
    mFontElements[10]->hudVisibility = HUDVisibility::OBJECT_META;
    mFontElements[11]->hudVisibility = HUDVisibility::OBJECT_META;
    mFontElements[12]->hudVisibility = HUDVisibility::OBJECT_META;
    
    for (UINT i = 13; i < mFontElements.size(); i++)
    {
        mFontElements[i]->hudVisibility = HUDVisibility::LIGHT;
    }

}

void EditModeHUD::update()
{
    auto dispSetting = ServiceProvider::getSettings()->displaySettings;
    auto editSetting = ServiceProvider::getEditSettings();

    /*show correct tool*/
    mHUDElements[3]->TexDescriptor = editSetting->toolMode == EditTool::Height ?
        TextureDescriptors::HEIGHT : editSetting->toolMode == EditTool::Paint ?
        TextureDescriptors::PAINT : editSetting->toolMode == EditTool::ObjectTransform ?
        TextureDescriptors::OBJECT : editSetting->toolMode == EditTool::ObjectMeta ?
        TextureDescriptors::OBJECT_META : editSetting->toolMode == EditTool::Light ?
        TextureDescriptors::LIGHT : TextureDescriptors::CAMERA;

    /*save window*/
    mHUDElements[5]->TexDescriptor = editSetting->saveSuccess ? TextureDescriptors::SAVED : TextureDescriptors::FAILED;

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

    if (editSetting->toolMode != EditTool::ObjectTransform && editSetting->toolMode != EditTool::ObjectMeta && editSetting->toolMode != EditTool::Camera && editSetting->toolMode != EditTool::Light)
    {
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
    }

    /*update for object mode*/
    else if(editSetting->toolMode == EditTool::ObjectMeta || editSetting->toolMode == EditTool::ObjectTransform || editSetting->toolMode == EditTool::Camera)
    {
        if (editSetting->toolMode == EditTool::ObjectTransform)
        {
            /*tool axis window*/
            mHUDElements[19]->NormalizedPosition.x = 0.8525f + (int)editSetting->objTransformTool * 0.052f;

            mHUDElements[20]->NormalizedPosition.y = 0.7675f + (int)editSetting->translationAxis * 0.033f;
            mHUDElements[21]->NormalizedPosition.y = 0.7675f + (int)editSetting->scaleAxis * 0.033f;
            mHUDElements[22]->NormalizedPosition.y = 0.7675f + (int)editSetting->rotationAxis * 0.033f;
        }
        else if (editSetting->toolMode == EditTool::ObjectMeta)
        {
            mFontElements[10]->text = d3dUtil::convertStringToWstring(editSetting->currentSelection->renderItem->Model->name);
            mFontElements[12]->text = L"Group: " + d3dUtil::convertStringToWstring(editSetting->selectedGroup);
            mFontElements[11]->text = editSetting->currentSelection->renderItem->renderType == RenderType::Default ?
                L"Mode: Default" : editSetting->currentSelection->renderItem->renderType == RenderType::DefaultAlpha ?
                L"Mode: DefaultAlpha" : editSetting->currentSelection->renderItem->renderType == RenderType::DefaultTransparency ?
                L"Mode: DefaultTransparency" : editSetting->currentSelection->renderItem->renderType == RenderType::DefaultNoNormal ?
                L"Mode: DefaultNoNormalMap" : editSetting->currentSelection->renderItem->renderType == RenderType::NoCullNoNormal ? 
                L"Mode: NoCullNoNormal" : L"Mode: unknown";

            mHUDElements[25]->NormalizedPosition.y = 0.8f + (int)editSetting->gameObjectProperty * 0.025f;

            mHUDElements[26]->TexDescriptor = editSetting->currentSelection->isCollisionEnabled ? TextureDescriptors::SUCCESS : TextureDescriptors::FAILED;
            mHUDElements[27]->TexDescriptor = editSetting->currentSelection->isDrawEnabled ? TextureDescriptors::SUCCESS : TextureDescriptors::FAILED;
            mHUDElements[28]->TexDescriptor = editSetting->currentSelection->isShadowEnabled ? TextureDescriptors::SUCCESS : TextureDescriptors::FAILED;
            mHUDElements[29]->TexDescriptor = editSetting->currentSelection->isShadowForced ? TextureDescriptors::SUCCESS : TextureDescriptors::FAILED;
        }


        /*object info window*/
        mFontElements[9]->text = d3dUtil::convertStringToWstring(editSetting->currentSelection->name);

        XMFLOAT3 p = editSetting->currentSelection->getPosition();
        XMFLOAT3 s = editSetting->currentSelection->getScale();
        XMFLOAT3 r = editSetting->currentSelection->getRotation();

        std::wstringstream ss;
        ss << std::setprecision(5) << p.x;
        mFontElements[0]->text = ss.str();

        ss.str(L"");
        ss << p.y;
        mFontElements[3]->text = ss.str();

        ss.str(L"");
        ss << p.z;
        mFontElements[6]->text = ss.str();

        ss.str(L"");
        ss << s.x;
        mFontElements[1]->text = ss.str();

        ss.str(L"");
        ss << s.y;
        mFontElements[4]->text = ss.str();

        ss.str(L"");
        ss << s.z;
        mFontElements[7]->text = ss.str();

        ss.str(L"");
        ss << XMConvertToDegrees(r.x) << "*";
        mFontElements[2]->text = ss.str();

        ss.str(L"");
        ss << XMConvertToDegrees(r.y) << "*";
        mFontElements[5]->text = ss.str();

        ss.str(L"");
        ss << XMConvertToDegrees(r.z) << "*";
        mFontElements[8]->text = ss.str();

    }
    else if (editSetting->toolMode == EditTool::Light)
    {
        mFontElements[13]->text = d3dUtil::convertStringToWstring(ServiceProvider::getActiveLevel()->mLightObjects[editSetting->currentLightSelectionIndex]->name);
        
        std::wstringstream ss;
        ss << std::setprecision(5) << ServiceProvider::getActiveLevel()->mLightObjects[editSetting->currentLightSelectionIndex]->getFallOffStart();
        mFontElements[23]->text = ss.str();

        ss.str(L"");
        ss << ServiceProvider::getActiveLevel()->mLightObjects[editSetting->currentLightSelectionIndex]->getFallOffEnd();
        mFontElements[24]->text = ss.str();

        XMFLOAT3 p = ServiceProvider::getActiveLevel()->mLightObjects[editSetting->currentLightSelectionIndex]->getPosition();
        XMFLOAT3 s = ServiceProvider::getActiveLevel()->mLightObjects[editSetting->currentLightSelectionIndex]->getStrength();
        XMFLOAT3 d = ServiceProvider::getActiveLevel()->mLightObjects[editSetting->currentLightSelectionIndex]->getDirection();

        ss.str(L"");
        ss << p.x;
        mFontElements[14]->text = ss.str();

        ss.str(L"");
        ss << p.y;
        mFontElements[15]->text = ss.str();

        ss.str(L"");
        ss << p.z;
        mFontElements[16]->text = ss.str();

        ss.str(L"");
        ss << s.x;
        mFontElements[17]->text = ss.str();

        ss.str(L"");
        ss << s.y;
        mFontElements[18]->text = ss.str();

        ss.str(L"");
        ss << s.z;
        mFontElements[19]->text = ss.str();

        ss.str(L"");
        ss << d.x;
        mFontElements[20]->text = ss.str();

        ss.str(L"");
        ss << d.y;
        mFontElements[21]->text = ss.str();

        ss.str(L"");
        ss << d.z;
        mFontElements[22]->text = ss.str();
    }

    /*recalculate actual screen position*/
    for (auto& e : mHUDElements)
    {
        e->ScreenPosition.x = e->NormalizedPosition.x * dispSetting.ResolutionWidth;
        e->ScreenPosition.y = e->NormalizedPosition.y * dispSetting.ResolutionHeight;
    }

    for (auto& e : mFontElements)
    {
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

    auto toolMode = ServiceProvider::getEditSettings()->toolMode;

    for (const auto& e : mHUDElements)
    {
        if (!e->Visible) continue;

        if (e->hudVisibility == HUDVisibility::HEIGHT_AND_PAINT && (toolMode == EditTool::ObjectTransform || toolMode == EditTool::Camera || toolMode == EditTool::ObjectMeta || toolMode == EditTool::Light)) continue;
        if (e->hudVisibility == HUDVisibility::HEIGHT && toolMode != EditTool::Height)continue;
        if (e->hudVisibility == HUDVisibility::PAINT && toolMode != EditTool::Paint)continue;
        if (e->hudVisibility == HUDVisibility::OBJECT && toolMode != EditTool::ObjectTransform)continue;
        if (e->hudVisibility == HUDVisibility::BOTH_OBJECT && (toolMode != EditTool::ObjectTransform && toolMode != EditTool::ObjectMeta && toolMode != EditTool::Camera)) continue;
        if (e->hudVisibility == HUDVisibility::OBJECT_META && toolMode != EditTool::ObjectMeta)continue;
        if (e->hudVisibility == HUDVisibility::FPS_CAMERA && toolMode != EditTool::Camera) continue;
        if (e->hudVisibility == HUDVisibility::LIGHT && toolMode != EditTool::Light) continue;

        mSpriteBatch->Draw(m_resourceDescriptors->GetGpuHandle(e->TexDescriptor),
                           e->TextureSize,
                           e->ScreenPosition,
                           &e->SourceRectangle,
                           Colors::White,
                           e->Rotation,
                           e->Origin,
                           e->ResolutionScale * e->Scale);
    }

    /*draw font after textures*/
    for (const auto& f : mFontElements)
    {
        if (!f->Visible) continue;
        if (f->hudVisibility == HUDVisibility::HEIGHT_AND_PAINT && (toolMode == EditTool::ObjectTransform || toolMode == EditTool::Camera)) continue;
        if (f->hudVisibility == HUDVisibility::HEIGHT && toolMode != EditTool::Height)continue;
        if (f->hudVisibility == HUDVisibility::PAINT && toolMode != EditTool::Paint)continue;
        if (f->hudVisibility == HUDVisibility::OBJECT && toolMode != EditTool::ObjectTransform)continue;
        if (f->hudVisibility == HUDVisibility::BOTH_OBJECT && (toolMode != EditTool::ObjectTransform && toolMode != EditTool::ObjectMeta && toolMode != EditTool::Camera)) continue;
        if (f->hudVisibility == HUDVisibility::OBJECT_META && toolMode != EditTool::ObjectMeta)continue;
        if (f->hudVisibility == HUDVisibility::LIGHT && toolMode != EditTool::Light)continue;

        mFonts[f->font]->DrawString(mSpriteBatch.get(),
                                    f->text.c_str(),
                                    f->ScreenPosition,
                                    Colors::RosyBrown,
                                    0.0f,
                                    f->Origin,
                                    f->ResolutionScale * f->Scale);
    }

    mSpriteBatch->End();

    /*draw texture of paint mode*/
    if (toolMode == EditTool::Paint)
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

std::unique_ptr<EditModeHUD::HUDElement> EditModeHUD::initHUDElement(TextureDescriptors desc, DirectX::SimpleMath::Vector2 nPos, float scaleF)
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

std::unique_ptr<EditModeHUD::FontElement> EditModeHUD::initFontElement(EditModeHUD::FontDescriptors desc, DirectX::SimpleMath::Vector2 nPos, float scaleF)
{
    auto element = std::make_unique<FontElement>();

    element->text = L"-";
    element->NormalizedPosition = nPos;
    element->ResolutionScale = resolutionScaleFactor * scaleF;

    return std::move(element);
}