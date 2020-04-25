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

private:
    std::unique_ptr<DirectX::DescriptorHeap> m_resourceDescriptors;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_texture;

    enum Descriptors
    {
        Cat,
        Count
    };
    std::unique_ptr<DirectX::SpriteBatch> m_spriteBatch;
    DirectX::SimpleMath::Vector2 m_screenPos;
    DirectX::SimpleMath::Vector2 m_origin;

    std::unique_ptr<DirectX::GraphicsMemory> m_graphicsMemory;
};