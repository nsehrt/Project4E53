#pragma once

#include "../core/editmode.h"
#include "SpriteBatch.h"
#include "DescriptorHeap.h"
#include "SimpleMath.h"
#include "GraphicsMemory.h"

class EditModeHUD
{
public:
    EditModeHUD();

    void init();
    void update();
    void draw();
    void commit();

private:
    std::unique_ptr<DirectX::DescriptorHeap> m_resourceDescriptors;
    std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> mTextures;

    const DirectX::SimpleMath::Vector2 saveWindowPos = { 0.5f, 0.05f };
    const DirectX::SimpleMath::Vector2 saveWindowIconPos = { 0.46f, 0.05f };
    const float saveWindowLinger = 1.0f;

    const DirectX::SimpleMath::Vector2 legendWindowPos = { 0.08f, 0.4f };

    const DirectX::SimpleMath::Vector2 sliderMinMax = { 0.046f, 0.204f };

    enum Descriptors
    {
        /*Xbox Buttons*/
        BUTTON_A,
        BUTTON_B,
        BUTTON_X,
        BUTTON_Y,
        BUTTON_LSHOULDER,
        BUTTON_RSHOULDER,
        BUTTON_LTRIGGER,
        BUTTON_RTRIGGER,
        BUTTON_BACK,
        BUTTON_MENU,
        BUTTON_DPAD,
        BUTTON_DPAD_DOWN,
        BUTTON_DPAD_UP,
        BUTTON_DPAD_LEFT,
        BUTTON_DPAD_RIGHT,
        BUTTON_LSTICK,
        BUTTON_RSTICK,
        /*GUI Elements*/
        TOOL_WIN,
        HEIGHT,
        PAINT,
        LEFT,
        RIGHT,
        SAVE_WIN,
        SAVED,
        FAILED,
        SELECT_WIN,
        SLIDER_FG_BLUE,
        SLIDER_FG_GREEN,
        SLIDER_FG_RED,
        SLIDER_BLUE,
        SLIDER_GREEN,
        SLIDER_RED,
        LEGEND_WIN,
        LEGEND_FULL_WIN,
        TEXTURE_WIN,
        OBJECT,
        Count
    };

    enum class HUDVisibility
    {
        ALWAYS,
        NOT_OBJECT,
        HEIGHT,
        PAINT,
        OBJECT
    };

    struct HUDElement
    {
        HUDElement(Descriptors a) : TexDescriptor(a){}

        Descriptors TexDescriptor = Descriptors::BUTTON_A;
        DirectX::SimpleMath::Vector2 NormalizedPosition = { 0.0f,0.0f };
        DirectX::SimpleMath::Vector2 ScreenPosition = { 0.0f,0.0f };
        DirectX::SimpleMath::Vector2 Origin = { 0.0f,0.0f };
        RECT SourceRectangle;
        DirectX::XMUINT2 TextureSize = { 0,0 };
        float ResolutionScale = 1.0f;
        float Scale = 1.0f;
        float Rotation = 0.0f;
        HUDVisibility hudVisibility = HUDVisibility::ALWAYS;
        bool Visible = true;
    };

    std::vector<std::unique_ptr<HUDElement>> mHUDElements;
    std::unique_ptr<HUDElement> mTexturePreview;
    std::unique_ptr<DirectX::SpriteBatch> mSpriteBatch;

    std::unique_ptr<DirectX::GraphicsMemory> mGraphicsMemory;

    const float DEFAULT_WIDTH = 1920.f;
    const float DEFAULT_HEIGHT = 1080.f;

    float resolutionScaleFactor = 1.0f;

    std::unique_ptr<HUDElement> initHUDElement(Descriptors desc, DirectX::SimpleMath::Vector2 nPos, float scaleF = 1.0f);

};