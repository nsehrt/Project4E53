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
        Count
    };

    struct HUDElement
    {
        HUDElement(Descriptors a) : TexDescriptor(a){}

        Descriptors TexDescriptor = Descriptors::BUTTON_A;
        DirectX::SimpleMath::Vector2 ScreenPosition = { 0.0f,0.0f };
        DirectX::SimpleMath::Vector2 Origin = { 0.0f,0.0f };
        DirectX::XMUINT2 TextureSize = { 0,0 };
        float ResolutionScale = 1.0f;
        float Scale = 1.0f;
        float Rotation = 0.0f;
        bool Visible = true;
    };

    std::vector<std::unique_ptr<HUDElement>> mHUDElements;
    std::unique_ptr<DirectX::SpriteBatch> mSpriteBatch;

    std::unique_ptr<DirectX::GraphicsMemory> mGraphicsMemory;

    const float DEFAULT_WIDTH = 1920.f;
};