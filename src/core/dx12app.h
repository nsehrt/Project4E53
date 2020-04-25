#pragma once

#include "../util/d3dUtil.h"
#include "gametime.h"

/*d3d12 libraries*/
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "D3D12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dxguid.lib")


class DX12App
{
protected:
    DX12App(HINSTANCE hInstance);
    DX12App(const DX12App& rhs) = delete;
    DX12App& operator=(const DX12App& rhs) = delete;
    virtual ~DX12App();

public:

    static DX12App* getApp();

    HINSTANCE getAppInst() const;
    HWND getMainWindowHandle()const;
    float getAspectRatio()const;

    void setFullscreen(bool value);
    bool changeWindowSize();
    void getPrimaryResolution();

    int run();

    virtual bool Initialize();
    virtual LRESULT MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

protected:
    virtual void createRtvAndDsvDescriptorHeaps();
    virtual void onResize();
    virtual void update(const GameTime& gt) = 0;
    virtual void draw(const GameTime& gt) = 0;

    bool initMainWindow();
    bool initDirect3D();
    void createCommandObjects();
    void createSwapChain();

    void flushCommandQueue();

    ID3D12Resource* getCurrentBackBuffer()const;
    D3D12_CPU_DESCRIPTOR_HANDLE getCurrentBackBufferView()const;
    D3D12_CPU_DESCRIPTOR_HANDLE getDepthStencilView()const;

    void calculateFrameStats();

    void logAdapters();
    //void logAdapterOutputs(IDXGIAdapter* adapter);
    //void logOutputDisplayModes(IDXGIOutput* output, DXGI_FORMAT format);

protected:
    static DX12App* dx12App;

    HINSTANCE mAppInst = nullptr;
    HWND mMainWindow = nullptr;
    bool mAppPaused = false;
    bool mMinimized = false;
    bool mMaximized = false;
    bool mResizing = false;
    bool mFullscreenState = false;

    GameTime mTimer;

    Microsoft::WRL::ComPtr<IDXGIFactory4> mdxgiFactory;
    Microsoft::WRL::ComPtr<IDXGISwapChain> mSwapChain;
    Microsoft::WRL::ComPtr<ID3D12Device> mDevice;

    Microsoft::WRL::ComPtr<ID3D12Fence> mFence;
    UINT64 mCurrentFence = 0;

    Microsoft::WRL::ComPtr<ID3D12CommandQueue> mCommandQueue;
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> mDirectCmdListAlloc;
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> mCommandList;

    static const int SwapChainBufferCount = 2;
    int mCurrBackBuffer = 0;

    Microsoft::WRL::ComPtr<ID3D12Resource> mSwapChainBuffer[SwapChainBufferCount];
    Microsoft::WRL::ComPtr<ID3D12Resource> mDepthStencilBuffer;

    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mRtvHeap;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mDsvHeap;

    D3D12_VIEWPORT mScreenViewport;
    D3D12_RECT mScissorRect;

    UINT mRtvDescriptorSize = 0;
    UINT mDsvDescriptorSize = 0;
    UINT mCbvSrvUavDescriptorSize = 0;


    std::wstring mWindowCaption = L"dx12 app";
    D3D_DRIVER_TYPE mDriverType = D3D_DRIVER_TYPE::D3D_DRIVER_TYPE_HARDWARE;
    DXGI_FORMAT mBackBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
    DXGI_FORMAT mDepthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
    int mWindowWidth = 1280;
    int mWindowHeight = 720;

    int priResX = 0;
    int priResY = 0;

};