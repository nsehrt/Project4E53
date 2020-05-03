#include "dx12app.h"
#include "../util/serviceprovider.h"
#include <windowsx.h>

using Microsoft::WRL::ComPtr;
using namespace DirectX;
using namespace std;

LRESULT CALLBACK MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    return DX12App::getApp()->MsgProc(hwnd, msg, wParam, lParam);
}

DX12App* DX12App::dx12App = nullptr;
DX12App* DX12App::getApp()
{
    return dx12App;
}

DX12App::DX12App(HINSTANCE /*hInstance*/)
{
    assert(dx12App == nullptr);
    dx12App = this;
    mScissorRect = D3D12_RECT();
    mScreenViewport = D3D12_VIEWPORT();

    mWindowWidth = 800;
    mWindowHeight = 600;

    getPrimaryResolution();
}

DX12App::~DX12App()
{
    if (mDevice != nullptr)
        flushCommandQueue();
}

HINSTANCE DX12App::getAppInst()const
{
    return mAppInst;
}

HWND DX12App::getMainWindowHandle()const
{
    return mMainWindow;
}

float DX12App::getAspectRatio()const
{
    return static_cast<float>(mWindowWidth) / mWindowHeight;
}

void DX12App::setFullscreen(bool value)
{
    if (value)
    {
        /*query for monitors*/
        IDXGIAdapter* adapter = nullptr;
        mdxgiFactory->EnumAdapters(0, &adapter);

        IDXGIOutput* output = nullptr;
        if (adapter->EnumOutputs(ServiceProvider::getSettings()->displaySettings.Monitor, &output) == DXGI_ERROR_NOT_FOUND)
        {
            HRESULT hr = mSwapChain->SetFullscreenState(true, nullptr);

            if (hr != S_OK)
            {
                LOG(Severity::Error, "Can't enter exclusive fullscreen, probably another window in exlusive fullscreen already exists!")
                    return;
            }

            ServiceProvider::getLogger()->print<Severity::Info>("Switching to fullscreen mode on standard monitor. (Setting ignored)");
        }
        else
        {
            HRESULT hr = mSwapChain->SetFullscreenState(true, output);

            if (hr != S_OK)
            {
                LOG(Severity::Error, "Can't enter exclusive fullscreen, probably another window in exlusive fullscreen already exists!")
                    return;
            }

            LOG(Severity::Info, "Switching to fullscreen mode on Monitor " << ServiceProvider::getSettings()->displaySettings.Monitor << ".");
        }
    }
    else
    {
        mSwapChain->SetFullscreenState(false, nullptr);

        ServiceProvider::getLogger()->print<Severity::Info>("Leaving fullscreen mode.");
    }
}

bool DX12App::changeWindowSize()
{
    int fX = (priResX - mWindowWidth) / 2;
    int fY = (priResY - mWindowHeight) / 2;

    if (fX < 0) fX = 0;
    if (fY < 0) fY = 0;

    bool ret = SetWindowPos(mMainWindow, HWND_TOP, fX, fY, mWindowWidth, mWindowHeight, SWP_SHOWWINDOW);

    return ret;
}

void DX12App::getPrimaryResolution()
{
    priResX = GetSystemMetrics(SM_CXSCREEN);
    priResY = GetSystemMetrics(SM_CYSCREEN);
}

int DX12App::run()
{
    MSG msg = { 0 };

    mTimer.Reset();

    while (msg.message != WM_QUIT)
    {
        if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else
        {
            mTimer.Tick();

            calculateFrameStats();
            update(mTimer);
            draw(mTimer);
        }
    }

    ServiceProvider::getLogger()->print<Severity::Info>("Main window has been flagged for destruction.");

    if (mFullscreenState)
    {
        setFullscreen(false);
    }

    return (int)msg.wParam;
}

bool DX12App::Initialize()
{
    if (!initMainWindow())
    {
        ServiceProvider::getLogger()->print<Severity::Error>("Failed to create main window!");
        return false;
    }
    else
    {
        ServiceProvider::getLogger()->print<Severity::Info>("Created main window successfully.");
    }

    if (!initDirect3D())
    {
        ServiceProvider::getLogger()->print<Severity::Error>("Failed to initialize DirectX 12!");
        return false;
    }
    else
    {
        ServiceProvider::getLogger()->print<Severity::Info>("DirectX 12 was set up successfully.");
    }

    /*fullscreen state*/

    mWindowWidth = ServiceProvider::getSettings()->displaySettings.ResolutionWidth;
    mWindowHeight = ServiceProvider::getSettings()->displaySettings.ResolutionHeight;

    changeWindowSize();

    mFullscreenState = ServiceProvider::getSettings()->displaySettings.WindowMode == 1 ? 1 : 0;
    setFullscreen(mFullscreenState);

    return true;
}

void DX12App::createRtvAndDsvDescriptorHeaps()
{
    D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc;
    rtvHeapDesc.NumDescriptors = SwapChainBufferCount;
    rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    rtvHeapDesc.NodeMask = 0;
    ThrowIfFailed(mDevice->CreateDescriptorHeap(
        &rtvHeapDesc, IID_PPV_ARGS(mRtvHeap.GetAddressOf())));

    D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc;
    dsvHeapDesc.NumDescriptors = 1;
    dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    dsvHeapDesc.NodeMask = 0;
    ThrowIfFailed(mDevice->CreateDescriptorHeap(
        &dsvHeapDesc, IID_PPV_ARGS(mDsvHeap.GetAddressOf())));
}

void DX12App::onResize()
{
    assert(mDevice);
    assert(mSwapChain);
    assert(mDirectCmdListAlloc);

    flushCommandQueue();

    ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));

    ServiceProvider::getLogger()->print<Severity::Info>("Resizing DX12 resources.");

    /*release the previous resources*/
    for (int i = 0; i < SwapChainBufferCount; ++i)
        mSwapChainBuffer[i].Reset();
    mDepthStencilBuffer.Reset();

    /*resize swap chain*/
    ThrowIfFailed(mSwapChain->ResizeBuffers(
        SwapChainBufferCount,
        mWindowWidth, mWindowHeight,
        mBackBufferFormat,
        DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH));

    mCurrBackBuffer = 0;

    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHeapHandle(mRtvHeap->GetCPUDescriptorHandleForHeapStart());
    for (UINT i = 0; i < SwapChainBufferCount; i++)
    {
        ThrowIfFailed(mSwapChain->GetBuffer(i, IID_PPV_ARGS(&mSwapChainBuffer[i])));
        mDevice->CreateRenderTargetView(mSwapChainBuffer[i].Get(), nullptr, rtvHeapHandle);
        rtvHeapHandle.Offset(1, mRtvDescriptorSize);
    }

    // Create the depth/stencil buffer and view.
    D3D12_RESOURCE_DESC depthStencilDesc;
    depthStencilDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    depthStencilDesc.Alignment = 0;
    depthStencilDesc.Width = mWindowWidth;
    depthStencilDesc.Height = mWindowHeight;
    depthStencilDesc.DepthOrArraySize = 1;
    depthStencilDesc.MipLevels = 1;
    depthStencilDesc.Format = mDepthStencilFormat;
    depthStencilDesc.SampleDesc.Count = 1;
    depthStencilDesc.SampleDesc.Quality = 0;
    depthStencilDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    depthStencilDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

    D3D12_CLEAR_VALUE optClear;
    optClear.Format = mDepthStencilFormat;
    optClear.DepthStencil.Depth = 1.0f;
    optClear.DepthStencil.Stencil = 0;
    ThrowIfFailed(mDevice->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
        D3D12_HEAP_FLAG_NONE,
        &depthStencilDesc,
        D3D12_RESOURCE_STATE_COMMON,
        &optClear,
        IID_PPV_ARGS(mDepthStencilBuffer.GetAddressOf())));

    // Create descriptor to mip level 0 of entire resource using the format of the resource.
    mDevice->CreateDepthStencilView(mDepthStencilBuffer.Get(), nullptr, getDepthStencilView());

    // Transition the resource from its initial state to be used as a depth buffer.
    mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mDepthStencilBuffer.Get(),
                                  D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_DEPTH_WRITE));

    // Execute the resize commands.
    ThrowIfFailed(mCommandList->Close());
    ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
    mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

    // Wait until resize is complete.
    flushCommandQueue();

    // Update the viewport transform to cover the client area.
    mScreenViewport.TopLeftX = 0;
    mScreenViewport.TopLeftY = 0;
    mScreenViewport.Width = static_cast<float>(mWindowWidth);
    mScreenViewport.Height = static_cast<float>(mWindowHeight);
    mScreenViewport.MinDepth = 0.0f;
    mScreenViewport.MaxDepth = 1.0f;

    mScissorRect = { 0, 0, mWindowWidth, mWindowHeight };
}

/***WIN32 Message Loop****/
LRESULT DX12App::MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
        // WM_ACTIVATE is sent when the window is activated or deactivated.
        // We pause the game when the window is deactivated and unpause it
        // when it becomes active.
        case WM_ACTIVATE:
            return 0;

            // WM_SIZE is sent when the user resizes the window.
        case WM_SIZE:
            if (mDevice)
            {
                onResize();
            }
            return 0;

            // WM_EXITSIZEMOVE is sent when the user grabs the resize bars.
        case WM_ENTERSIZEMOVE:
            return 0;

            // WM_EXITSIZEMOVE is sent when the user releases the resize bars.
            // Here we reset everything based on the new window dimensions.
        case WM_EXITSIZEMOVE:
            return 0;

            // WM_DESTROY is sent when the window is being destroyed.
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;

            // The WM_MENUCHAR message is sent when a menu is active and the user presses
            // a key that does not correspond to any mnemonic or accelerator key.
        case WM_MENUCHAR:
            // Don't beep when we alt-enter.
            return MAKELRESULT(0, MNC_CLOSE);

            // Catch this message so to prevent the window from becoming too small.
        case WM_GETMINMAXINFO:
            return 0;

        case WM_KEYUP:
            if (wParam == VK_ESCAPE)
            {
                PostQuitMessage(0);
            }

            return 0;
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}

bool DX12App::initMainWindow()
{
    WNDCLASS wc;
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = MainWndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = mAppInst;
    wc.hIcon = LoadIcon(0, IDI_APPLICATION);
    wc.hCursor = LoadCursor(0, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
    wc.lpszMenuName = 0;
    wc.lpszClassName = L"MainWnd";

    if (!RegisterClass(&wc))
    {
        MessageBox(0, L"RegisterClass Failed.", 0, 0);
        return false;
    }

    // Compute window rectangle dimensions based on requested client area dimensions.
    RECT R = { 0, 0, mWindowWidth, mWindowHeight };
    AdjustWindowRect(&R, WS_OVERLAPPEDWINDOW, false);
    int width = R.right - R.left;
    int height = R.bottom - R.top;

    if (ServiceProvider::getSettings()->displaySettings.WindowMode != 2)
    {
        mMainWindow = CreateWindow(L"MainWnd", mWindowCaption.c_str(),
                                   WS_OVERLAPPEDWINDOW ^ WS_THICKFRAME ^ WS_MAXIMIZEBOX ^ WS_MINIMIZEBOX, CW_USEDEFAULT, CW_USEDEFAULT, width, height, 0, 0, mAppInst, 0);
    }
    else
    {
        mMainWindow = CreateWindow(L"MainWnd", mWindowCaption.c_str(),
                                   WS_EX_TOPMOST | WS_POPUP, CW_USEDEFAULT, CW_USEDEFAULT, width, height, 0, 0, mAppInst, 0);
    }

    if (!mMainWindow)
    {
        MessageBox(0, L"CreateWindow Failed.", 0, 0);
        return false;
    }

    CreateMutexA(0, false, "___dx12app___");
    if (GetLastError() == ERROR_ALREADY_EXISTS)
    {
        MessageBox(mMainWindow, L"Application already running!", L"Error", MB_OK);
        return 0;
    }

    ShowWindow(mMainWindow, SW_SHOW);
    UpdateWindow(mMainWindow);

    return true;
}

bool DX12App::initDirect3D()
{
#ifdef _DEBUG
    {
        ComPtr<ID3D12Debug> debugController;
        ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)));
        debugController->EnableDebugLayer();
    }
#endif

    ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(&mdxgiFactory)));

    // Try to create hardware device.
    HRESULT hardwareResult = D3D12CreateDevice(
        nullptr,             // default adapter
        D3D_FEATURE_LEVEL_11_0,
        IID_PPV_ARGS(&mDevice)
    );

    // Fallback to WARP device.
    if (FAILED(hardwareResult))
    {
        ServiceProvider::getLogger()->print<Severity::Warning>("Failed to create graphics device, falling back to Warp!");

        ComPtr<IDXGIAdapter> pWarpAdapter;
        ThrowIfFailed(mdxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(&pWarpAdapter)));

        ThrowIfFailed(D3D12CreateDevice(
            pWarpAdapter.Get(),
            D3D_FEATURE_LEVEL_11_0,
            IID_PPV_ARGS(&mDevice)));
    }

    ThrowIfFailed(mDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE,
                  IID_PPV_ARGS(&mFence)));

    mRtvDescriptorSize = mDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    mDsvDescriptorSize = mDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
    mCbvSrvUavDescriptorSize = mDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    logAdapters();

    LOG(Severity::Info, "Using graphic adapter " << ServiceProvider::getSettings()->miscSettings.AdapterName << ".");

    createCommandObjects();
    createSwapChain();
    createRtvAndDsvDescriptorHeaps();

    /*disable alt enter*/
    mdxgiFactory->MakeWindowAssociation(mMainWindow, DXGI_MWA_NO_ALT_ENTER | DXGI_MWA_NO_WINDOW_CHANGES);

    return true;
}

void DX12App::createCommandObjects()
{
    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;

    ThrowIfFailed(mDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&mCommandQueue)));

    ThrowIfFailed(mDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(mDirectCmdListAlloc.GetAddressOf())));

    ThrowIfFailed(mDevice->CreateCommandList(
        0,
        D3D12_COMMAND_LIST_TYPE_DIRECT,
        mDirectCmdListAlloc.Get(),
        nullptr,
        IID_PPV_ARGS(mCommandList.GetAddressOf())));

    mCommandList->Close();
}

void DX12App::createSwapChain()
{
    // Release the previous swapchain we will be recreating.
    mSwapChain.Reset();

    DXGI_SWAP_CHAIN_DESC sd;
    sd.BufferDesc.Width = mWindowWidth;
    sd.BufferDesc.Height = mWindowHeight;
    sd.BufferDesc.RefreshRate.Numerator = (int)ServiceProvider::getSettings()->displaySettings.RefreshRate;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.BufferDesc.Format = mBackBufferFormat;
    sd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    sd.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.BufferCount = SwapChainBufferCount;
    sd.OutputWindow = mMainWindow;
    sd.Windowed = true;
    sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

    // Note: Swap chain uses queue to perform flush.
    ThrowIfFailed(mdxgiFactory->CreateSwapChain(
        mCommandQueue.Get(),
        &sd,
        mSwapChain.GetAddressOf()));
}

void DX12App::flushCommandQueue()
{
    mCurrentFence++;

    ThrowIfFailed(mCommandQueue->Signal(mFence.Get(), mCurrentFence));

    if (mFence->GetCompletedValue() < mCurrentFence)
    {
        HANDLE eventHandle = CreateEventEx(nullptr, nullptr, FALSE, EVENT_ALL_ACCESS);

        ThrowIfFailed(mFence->SetEventOnCompletion(mCurrentFence, eventHandle));

        if (eventHandle != NULL)
        {
            WaitForSingleObject(eventHandle, INFINITE);
            CloseHandle(eventHandle);
        }
    }
}

ID3D12Resource* DX12App::getCurrentBackBuffer()const
{
    return mSwapChainBuffer[mCurrBackBuffer].Get();
}

D3D12_CPU_DESCRIPTOR_HANDLE DX12App::getCurrentBackBufferView()const
{
    return CD3DX12_CPU_DESCRIPTOR_HANDLE(
        mRtvHeap->GetCPUDescriptorHandleForHeapStart(),
        mCurrBackBuffer,
        mRtvDescriptorSize);
}

D3D12_CPU_DESCRIPTOR_HANDLE DX12App::getDepthStencilView()const
{
    return mDsvHeap->GetCPUDescriptorHandleForHeapStart();
}

void DX12App::calculateFrameStats()
{
    static int frameCount = 0;
    static float timeElapsed = 0.0f;

    frameCount++;

    if ((mTimer.TotalTime() - timeElapsed) >= 0.25f)
    {
        float fps = (float)frameCount * 4;
        float mspf = 250.0f / fps;

        wstring fpsStr = to_wstring(fps);
        wstring mspfStr = to_wstring(mspf);
        wstring objectsDrawn = to_wstring(ServiceProvider::getDebugInfo()->DrawnGameObjects);
        wstring shadowsDrawn = to_wstring(ServiceProvider::getDebugInfo()->DrawnShadowObjects);

        wstring windowText = mWindowCaption +
            L"    fps: " + fpsStr +
            L"   mspf: " + mspfStr + L"  objects drawn: " + objectsDrawn + L"  shadows drawn: " + shadowsDrawn;
        SetWindowText(mMainWindow, windowText.c_str());

        frameCount = 0;
        timeElapsed += 0.25f;
    }
}

void DX12App::logAdapters()
{
    UINT i = 0;
    IDXGIAdapter* adapter = nullptr;
    std::vector<IDXGIAdapter*> adapterList;
    while (mdxgiFactory->EnumAdapters(i, &adapter) != DXGI_ERROR_NOT_FOUND)
    {
        DXGI_ADAPTER_DESC desc;
        adapter->GetDesc(&desc);

        if (ServiceProvider::getSettings()->miscSettings.AdapterName == "")
        {
            std::wstring t = desc.Description;
            std::string s(t.begin(), t.end());
            ServiceProvider::getSettings()->miscSettings.AdapterName = s;
        }

        ReleaseCom(adapter);

        i++;
    }
}