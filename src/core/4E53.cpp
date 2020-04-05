#pragma warning(disable:4244)
#pragma warning(disable:26812)
#pragma warning(disable:26451)
#pragma warning(disable:6319)
#pragma warning(disable:6011)

#include <Windows.h>
#include "dx12app.h"
#include "../util/settings.h"
#include "../util/serviceprovider.h"
#include "../render/renderresource.h"
#include "../core/camera.h"
#include "../core/fpscamera.h"
#include "../util/modelloader.h"
#include "../core/level.h"
#include "../render/shadowmap.h"
#include <filesystem>

#ifndef _DEBUG
#define SETTINGS_FILE "cfg/settings.xml"
#else
#define SETTINGS_FILE "cfg/dbg.xml"
#endif
using namespace DirectX;

class P_4E53 : public DX12App
{

public:
	P_4E53(HINSTANCE hInstance);
	~P_4E53();

	virtual bool Initialize()override;

private:

	virtual void createRtvAndDsvDescriptorHeaps()override;
	virtual void onResize()override;
	virtual void update(const GameTime& gt)override;
	virtual void draw(const GameTime& gt)override;

	int vsyncIntervall = 0;

	std::unique_ptr<std::thread> inputThread;
	std::unique_ptr<std::thread> audioThread;

	std::shared_ptr<FPSCamera> fpsCamera;
	bool fpsCameraMode = false;

	std::vector<std::shared_ptr<Level>> mLevel;

	std::shared_ptr<ShadowMap> mShadowMap;
};

int gNumFrameResources = 3;

/*******/
/*main entry point*/
/*******/

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE /*hPrevInstance*/, 
				   _In_ LPSTR /*lpCmdLine*/, _In_ int /*nCmdShow*/)
{
#ifdef _DEBUG
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	_CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_DEBUG);
#endif

	auto startTime = std::chrono::system_clock::now();

	int status = 0;

	/*rand*/
	srand((unsigned int)time(NULL));

	/*create logger*/
	std::shared_ptr<Logger<LogPolicy>> vsLogger(new Logger<LogPolicy>(L""));
	vsLogger->setThreadName("mainThread");
	ServiceProvider::setLoggingService(vsLogger);

	ServiceProvider::getLogger()->print<Severity::Info>("Logger started successfully.");

	/*load settings file*/
	SettingsLoader settingsLoader;

	if (!settingsLoader.loadSettings(SETTINGS_FILE))
	{
		ServiceProvider::getLogger()->print<Severity::Error>("Failed to load settings.xml!");
		MessageBox(nullptr, L"Failed to load settings.xml!", L"Error", MB_OK);
		return -1;
	}
	ServiceProvider::setSettings(settingsLoader.get());

	ServiceProvider::getLogger()->print<Severity::Info>("Settings file loaded successfully.");

	/*initialize main window and directx12*/
	try
	{
		P_4E53 app(hInstance);
		if (!app.Initialize())
		{
			return 0;
		}

		LOG(Severity::Info, "Debug Mode is " << (ServiceProvider::getSettings()->miscSettings.DebugEnabled ? "enabled" : "disabled") << ".");

		/*wait for audio loading*/
		while (!ServiceProvider::getAudio()->loadingFinished())
			Sleep(5);

		auto endTime = std::chrono::system_clock::now();
		std::chrono::duration<double> elapsedTime = endTime - startTime;

		LOG(Severity::Info, "Game initialization was successful. (" << elapsedTime.count() << " seconds)");

		status = app.run();

		ServiceProvider::getLogger()->print<Severity::Info>("Game loop has been quit!");

		return 0;
	}
	catch (DxException & e)
	{
		ServiceProvider::getLogger()->print<Severity::Critical>("Exception thrown!");

		MessageBox(nullptr, e.toString().c_str(), L"HR Failed", MB_OK);
		status = -1;

		return status;
	}

}




/*class members*/

P_4E53::P_4E53(HINSTANCE hInstance)
	: DX12App(hInstance)
{
	mWindowCaption = L"Project 4E53";



}

P_4E53::~P_4E53()
{
	/*clean up*/

	ServiceProvider::getInputManager()->Stop();
	inputThread->join();

	ServiceProvider::getAudio()->Stop();
	audioThread->join();

	ServiceProvider::getAudio()->uninit();

	if (mDevice != nullptr)
		flushCommandQueue();
}



/*****************/
/*  Initalize   */
/****************/
bool P_4E53::Initialize()
{
	if (!DX12App::Initialize())
		return false;

	// Reset the command list to prep for initialization commands.
	ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));

	/**copy settings **/
	vsyncIntervall = ServiceProvider::getSettings()->displaySettings.VSync;
	gNumFrameResources = ServiceProvider::getSettings()->graphicSettings.numFrameResources;

	/*initialize input manager*/
	std::shared_ptr<InputManager> inputManager(new InputManager());
	ServiceProvider::setInputManager(inputManager);

	inputThread = std::make_unique<std::thread>(&InputManager::Loop, inputManager);

	/*initialize audio engine*/
	std::shared_ptr<SoundEngine> soundEngine(new SoundEngine());
	ServiceProvider::setAudioEngine(soundEngine);

	soundEngine->init();

	/*start the audio thread*/
	audioThread = std::make_unique<std::thread>(&SoundEngine::run, soundEngine);


	/*build dx12 resources*/
	std::filesystem::path texturePath(TEXTURE_PATH);
	std::filesystem::path modelPath(MODEL_PATH);

	if (!std::filesystem::exists(texturePath))
	{
		ServiceProvider::getLogger()->print<Severity::Critical>("Unable to access texture folder!");
		return false;
	}

	if (!std::filesystem::exists(modelPath))
	{
		ServiceProvider::getLogger()->print<Severity::Critical>("Unable to access model folder!");
		return false;
	}

	/*init shadow map*/
	mShadowMap = std::make_shared<ShadowMap>(mDevice.Get(), (UINT)ServiceProvider::getSettings()->graphicSettings.ShadowQuality,
											 (UINT)ServiceProvider::getSettings()->graphicSettings.ShadowQuality);

	ServiceProvider::setShadowMap(mShadowMap);

	/*initialize and register render resource*/
	std::shared_ptr<RenderResource> renderResource(new RenderResource());

	if (!renderResource->init(mDevice.Get(), mCommandList.Get(), texturePath, modelPath))
	{
		ServiceProvider::getLogger()->print<Severity::Error>("Initialising render resouce failed!");
		return false;
	}
	
	ServiceProvider::setRenderResource(renderResource);

	/*load first level*/
	auto level = std::make_shared<Level>();

	if (!level->load("0.level"))
	{
		
		return 0;
	}

	mLevel.push_back(std::move(level));
	ServiceProvider::setActiveLevel(mLevel.back());

	/*init fpscamera*/
	fpsCamera = std::make_shared<FPSCamera>();
	fpsCamera->setLens();
	fpsCamera->setPosition(0.0f, 5.0f, -20.f);

	// Execute the initialization commands.
	ThrowIfFailed(mCommandList->Close());
	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	// Wait until initialization is complete.
	flushCommandQueue();

	return true;
}

void P_4E53::createRtvAndDsvDescriptorHeaps()
{
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc;
	rtvHeapDesc.NumDescriptors = SwapChainBufferCount;
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	rtvHeapDesc.NodeMask = 0;
	ThrowIfFailed(mDevice->CreateDescriptorHeap(
		&rtvHeapDesc, IID_PPV_ARGS(mRtvHeap.GetAddressOf())));

	// Add +1 DSV for shadow map.
	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc;
	dsvHeapDesc.NumDescriptors = SwapChainBufferCount;
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	dsvHeapDesc.NodeMask = 0;
	ThrowIfFailed(mDevice->CreateDescriptorHeap(
		&dsvHeapDesc, IID_PPV_ARGS(mDsvHeap.GetAddressOf())));
}

void P_4E53::onResize()
{
	DX12App::onResize();
	if (ServiceProvider::getActiveCamera() != nullptr)
	{
		ServiceProvider::getActiveCamera()->setLens();
	}

}


/*=====================*/
/*  Update  */
/*=====================*/
void P_4E53::update(const GameTime& gt)
{
	/******************************/
	/*cycle to next frame resource*/
	auto renderResource = ServiceProvider::getRenderResource();

	ServiceProvider::getActiveLevel()->cycleFrameResource();

	//renderResource->incFrameResource();

	FrameResource* mCurrentFrameResource = ServiceProvider::getActiveLevel()->getCurrentFrameResource();
	/*wait for gpu if necessary*/
	if (mCurrentFrameResource->Fence != 0 && mFence->GetCompletedValue() < mCurrentFrameResource->Fence)
	{
		HANDLE eventHandle = CreateEventExW(nullptr, NULL, false, EVENT_ALL_ACCESS);
		ThrowIfFailed(mFence->SetEventOnCompletion(mCurrentFrameResource->Fence, eventHandle));
		WaitForSingleObject(eventHandle, INFINITE);
		CloseHandle(eventHandle);
	}

	/****************************/



	/*get input and settings*/
	InputSet& inputData = ServiceProvider::getInputManager()->getInput();
	Settings* settingsData = ServiceProvider::getSettings();
	/***********************/

	ServiceProvider::getActiveCamera()->mPreviousPosition = ServiceProvider::getActiveCamera()->getPosition3f();

	if (inputData.Pressed(BTN::A))
	{
		ServiceProvider::getAudio()->add(ServiceProvider::getAudioGuid(), "action");
	}

	/*fps camera controls*/
	if (inputData.Released(BTN::LEFT_THUMB))
	{
		fpsCameraMode = !fpsCameraMode;

		if (fpsCameraMode)
		{
			ServiceProvider::setActiveCamera(fpsCamera);
		}
		else
		{
			ServiceProvider::setActiveCamera(ServiceProvider::getActiveLevel()->mCameras[0]);
		}
	}

	if (fpsCameraMode)
	{
		fpsCamera->updateFPSCamera(inputData.current, gt);
	}
		
	if (inputData.Released(BTN::RIGHT_THUMB) && settingsData->miscSettings.DebugEnabled )
	{
		renderResource->toggleHitBoxDraw();
	}

	ServiceProvider::getActiveLevel()->update(gt);

	ServiceProvider::getActiveCamera()->updateViewMatrix();
	ServiceProvider::getActiveLevel()->updateBuffers(gt);

	/*save input for next frame*/
	ServiceProvider::getInputManager()->setPrevious(inputData.current);
	ServiceProvider::getInputManager()->releaseInput();
}


/*=====================*/
/*  Draw  */
/*=====================*/
void P_4E53::draw(const GameTime& gt)
{
	auto mCurrentFrameResource = ServiceProvider::getActiveLevel()->getCurrentFrameResource();
	auto renderResource = ServiceProvider::getRenderResource();

	auto cmdListAlloc = mCurrentFrameResource->CmdListAlloc;
	ThrowIfFailed(cmdListAlloc->Reset());

	mCommandList->Reset(
		cmdListAlloc.Get(),
		ServiceProvider::getRenderResource()->mPSOs["default"].Get()
	);

	/**/
	//ID3D12DescriptorHeap* descriptorHeaps[] = { renderResource->mSrvDescriptorHeap.Get() };
	//mCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	//mCommandList->SetGraphicsRootSignature(renderResource->mMainRootSignature.Get());

	//auto matBuffer = mCurrentFrameResource->MaterialBuffer->getResource();
	//mCommandList->SetGraphicsRootShaderResourceView(2, matBuffer->GetGPUVirtualAddress());
	//mCommandList->SetGraphicsRootDescriptorTable(3, renderResource->mNullSrv);

	//mCommandList->SetGraphicsRootDescriptorTable(4, renderResource->mSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());


	//mCommandList->RSSetViewports(1, &mShadowMap->getViewPort());
	//mCommandList->RSSetScissorRects(1, &mShadowMap->getScissor());

	//// Change to DEPTH_WRITE.
	//mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mShadowMap->getResource(),
	//							  D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_DEPTH_WRITE));

	//UINT passCBByteSize = d3dUtil::CalcConstantBufferSize(sizeof(PassConstants));

	//// Clear the back buffer and depth buffer.
	//mCommandList->ClearDepthStencilView(mShadowMap->getDsv(),
	//									D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

	//// Set null render target because we are only going to draw to
	//// depth buffer.  Setting a null render target will disable color writes.
	//// Note the active PSO also must specify a render target count of 0.
	//mCommandList->OMSetRenderTargets(0, nullptr, false, &mShadowMap->getDsv());

	//// Bind the pass constant buffer for the shadow map pass.
	//auto passCB = mCurrentFrameResource->PassCB->getResource();
	//D3D12_GPU_VIRTUAL_ADDRESS passCBAddress = passCB->GetGPUVirtualAddress() + 1 * passCBByteSize;
	//mCommandList->SetGraphicsRootConstantBufferView(1, passCBAddress);

	////mCommandList->SetPipelineState(renderResource->mPSOs["shadow"].Get());

	//ServiceProvider::getActiveLevel()->drawShadow();

	//// Change back to GENERIC_READ so we can read the texture in a shader.
	//mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mShadowMap->getResource(),
	//							  D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_GENERIC_READ));



	/**/
	mCommandList->RSSetViewports(1, &mScreenViewport);
	mCommandList->RSSetScissorRects(1, &mScissorRect);

	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(getCurrentBackBuffer(),
								  D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	/*clear buffers*/
	mCommandList->ClearRenderTargetView(getCurrentBackBufferView(), Colors::LimeGreen, 0, nullptr);
	mCommandList->ClearDepthStencilView(getDepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

	/*set render target*/
	mCommandList->OMSetRenderTargets(1, &getCurrentBackBufferView(), true, &getDepthStencilView());

	ID3D12DescriptorHeap* descHeap[] = { ServiceProvider::getRenderResource()->mSrvDescriptorHeap.Get() };
	mCommandList->SetDescriptorHeaps(_countof(descHeap), descHeap);

	/*set used root signature*/
	mCommandList->SetGraphicsRootSignature(ServiceProvider::getRenderResource()->mMainRootSignature.Get());

	/*set per pass constant buffer*/
	auto passCB = mCurrentFrameResource->PassCB->getResource();
	mCommandList->SetGraphicsRootConstantBufferView(1, passCB->GetGPUVirtualAddress());

	auto matBuffer = mCurrentFrameResource->MaterialBuffer->getResource();
	mCommandList->SetGraphicsRootShaderResourceView(2, matBuffer->GetGPUVirtualAddress());

	/*cubemap*/
	CD3DX12_GPU_DESCRIPTOR_HANDLE skyTexDescriptor(ServiceProvider::getRenderResource()->mSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
	skyTexDescriptor.Offset(ServiceProvider::getRenderResource()->mTextures["grasscube1024.dds"]->index, ServiceProvider::getRenderResource()->mHeapDescriptorSize);
	mCommandList->SetGraphicsRootDescriptorTable(3, skyTexDescriptor);

	/*textures (array)*/
	mCommandList->SetGraphicsRootDescriptorTable(4, ServiceProvider::getRenderResource()->mSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());

	/*draw everything*/
	
	ServiceProvider::getActiveLevel()->draw();

	/*to resource stage*/
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(getCurrentBackBuffer(),
								  D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

	// done
	ThrowIfFailed(mCommandList->Close());

	// add commands to queue
	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	// swap buffers in swapchain
	ThrowIfFailed(mSwapChain->Present(vsyncIntervall, 0));
	mCurrBackBuffer = (mCurrBackBuffer + 1) % SwapChainBufferCount;

	/*advance fence on gpu to signal that this frame is finished*/
	ServiceProvider::getActiveLevel()->getCurrentFrameResource()->Fence = ++mCurrentFence;
	mCommandQueue->Signal(mFence.Get(), mCurrentFence);

}
