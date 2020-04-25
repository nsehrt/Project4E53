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
#include "../core/fixedcamera.h"
#include "../util/modelloader.h"
#include "../core/level.h"
#include "../hud/editmodehud.h"
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

	std::unique_ptr<EditModeHUD> editModeHUD = nullptr;

	std::shared_ptr<FPSCamera> fpsCamera;
	bool fpsCameraMode = false;

	std::shared_ptr<FixedCamera> editCamera;
	LightObject* editLight = nullptr;

	std::vector<std::shared_ptr<Level>> mLevel;

	void drawToShadowMap();

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

	LOG(Severity::Info, "Debug Mode is " << (ServiceProvider::getSettings()->miscSettings.DebugEnabled ? "enabled" : "disabled") << ".");

	LOG(Severity::Info, "Edit Mode is " << (ServiceProvider::getSettings()->miscSettings.EditModeEnabled ? "enabled" : "disabled") << ".");


	/*initialize main window and directx12*/
	try
	{
		P_4E53 app(hInstance);
		if (!app.Initialize())
		{
			return 0;
		}

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

	/*initialize and register render resource*/
	std::shared_ptr<RenderResource> renderResource(new RenderResource(mRtvHeap, mDsvHeap));

	if (!renderResource->init(mDevice.Get(), mCommandList.Get(), texturePath, modelPath))
	{
		ServiceProvider::getLogger()->print<Severity::Error>("Initialising render resouce failed!");
		return false;
	}
	renderResource->cmdQueue = mCommandQueue.Get();

	ServiceProvider::setRenderResource(renderResource);

	/*load first level*/
	auto level = std::make_shared<Level>();

	if (!level->load("0.level"))
	{
		
		return 0;
	}

	mLevel.push_back(std::move(level));
	ServiceProvider::setActiveLevel(mLevel.back());

	editModeHUD = std::make_unique<EditModeHUD>();
	editModeHUD->init();

	/*init fpscamera*/
	fpsCamera = std::make_shared<FPSCamera>();
	fpsCamera->setLens();
	fpsCamera->setPosition(0.0f, 5.0f, -20.f);

	/*init edit mode camera if needed*/
	if (ServiceProvider::getSettings()->miscSettings.EditModeEnabled)
	{
		editCamera = std::make_shared<FixedCamera>();
		editCamera->initFixedDistance(15.0f, 100.0f);
		editCamera->setLens();
		editCamera->updateFixedCamera(XMFLOAT3(0.0f, 0.0f, 0.0f), 0.0f);
		ServiceProvider::setActiveCamera(editCamera);

		editLight = ServiceProvider::getActiveLevel()->mLightObjects[3].get();
		editLight->setStrength(XMFLOAT3(1.0f, 1.0f, 1.0f));
	}


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
	rtvHeapDesc.NumDescriptors = SwapChainBufferCount + 1; /*+ 1 off screen render target*/
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	rtvHeapDesc.NodeMask = 0;
	ThrowIfFailed(mDevice->CreateDescriptorHeap(
		&rtvHeapDesc, IID_PPV_ARGS(mRtvHeap.GetAddressOf())));

	
	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc;
	dsvHeapDesc.NumDescriptors = 2; /*+ 1 shadow map dsv*/
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

	auto renderResource = ServiceProvider::getRenderResource();
	if (renderResource == nullptr)return;

	if (renderResource->getSobelFilter() != nullptr)
	{
		renderResource->getSobelFilter()->onResize(ServiceProvider::getSettings()->displaySettings.ResolutionWidth, 
												 ServiceProvider::getSettings()->displaySettings.ResolutionHeight);
	}

	if (renderResource->getRenderTarget() != nullptr)
	{
		renderResource->getRenderTarget()->onResize(ServiceProvider::getSettings()->displaySettings.ResolutionWidth,
												   ServiceProvider::getSettings()->displaySettings.ResolutionHeight);
	}

	if (renderResource->getShadowMap() != nullptr)
	{
		renderResource->getShadowMap()->OnResize(ServiceProvider::getSettings()->displaySettings.ResolutionWidth,
												 ServiceProvider::getSettings()->displaySettings.ResolutionHeight);
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
	auto activeLevel = ServiceProvider::getActiveLevel();
	auto activeCamera = ServiceProvider::getActiveCamera();

	renderResource->cycleFrameResource();
	FrameResource* mCurrentFrameResource = renderResource->getCurrentFrameResource();

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


	activeCamera->mPreviousPosition = activeCamera->getPosition3f();
	/***********************/

	if (settingsData->miscSettings.EditModeEnabled)
	{
		auto editSettings = ServiceProvider::getEditSettings();

		/*save map*/
		if (inputData.Released(BTN::BACK))
		{
			activeLevel->mTerrain->save();
		}

		/*switch tool*/
		if (inputData.Pressed(BTN::RIGHT_SHOULDER))
		{
			if (editSettings->toolMode == EditTool::Height)
			{
				editSettings->toolMode = EditTool::Paint;
			}
			else
			{
				editSettings->toolMode = EditTool::Height;
			}
		}

		/*wireframe on/off*/
		if (inputData.Pressed(BTN::LEFT_THUMB))
		{
			editSettings->WireFrameOn = !editSettings->WireFrameOn;
		}


		/*edit selection update*/
		editSettings->Velocity = editSettings->BaseVelocity * editCamera->cameraPosNormalize();

		editSettings->Position.x += inputData.current.trigger[TRG::THUMB_LX] * editSettings->Velocity * gt.DeltaTime();
		editSettings->Position.y += inputData.current.trigger[TRG::THUMB_LY] * editSettings->Velocity * gt.DeltaTime();

		float terrainHalf = activeLevel->mTerrain->terrainSize / 2.0f;

		editSettings->Position.x = MathHelper::clampH(editSettings->Position.x,
													  -terrainHalf + 5.0f,
													  terrainHalf - 5.0f);

		editSettings->Position.y = MathHelper::clampH(editSettings->Position.y,
													  -terrainHalf + 5.0f,
													  terrainHalf - 5.0f);


		/*falloff radius control*/
		editSettings->FallOffRatio += inputData.current.trigger[TRG::THUMB_RX] * gt.DeltaTime() * (editSettings->fallOffRatioMax);
		editSettings->FallOffRatio = MathHelper::clampH(editSettings->FallOffRatio, editSettings->fallOffRatioMin,
														editSettings->fallOffRatioMax);

		editSettings->BaseRadius = editCamera->cameraPosNormalize() * (editCamera->cameraPosNormalize() * editSettings->BaseSelectSize);
		editSettings->FallOffRadius = editSettings->BaseRadius * editSettings->FallOffRatio;

		/*control for height tool*/
		if (editSettings->toolMode == EditTool::Height)
		{
			/*reset height save*/
			if (inputData.Pressed(BTN::RIGHT_THUMB))
			{
				editSettings->resetHeight = activeLevel->mTerrain->getHeight(editSettings->Position.x,
																			 editSettings->Position.y);
			}

			/*process height increase*/

			if (inputData.current.trigger[TRG::RIGHT_TRIGGER] > 0.15f)
			{
				activeLevel->mTerrain->increaseHeight(editSettings->Position.x,
													  editSettings->Position.y,
													  editSettings->FallOffRadius,
													  editSettings->BaseRadius,
													  gt.DeltaTime() * editSettings->heightIncrease * inputData.current.trigger[TRG::RIGHT_TRIGGER],
													  editSettings->resetHeight,
													  inputData.current.buttons[BTN::Y]);
			}

			if (inputData.current.trigger[TRG::LEFT_TRIGGER] > 0.15f)
			{
				activeLevel->mTerrain->increaseHeight(editSettings->Position.x,
													  editSettings->Position.y,
													  editSettings->FallOffRadius,
													  editSettings->BaseRadius,
													  gt.DeltaTime() * -editSettings->heightIncrease * inputData.current.trigger[TRG::LEFT_TRIGGER],
													  editSettings->resetHeight,
													  inputData.current.buttons[BTN::Y]);
			}

			/*control increase value*/
			if (inputData.current.buttons[BTN::B])
			{
				float newIncrease = editSettings->heightIncrease + gt.DeltaTime() * (editSettings->heightIncreaseMax / 4.0f);
				editSettings->heightIncrease = MathHelper::clampH(newIncrease, editSettings->heightIncreaseMin,
																 editSettings->heightIncreaseMax);
			}
			else if (inputData.current.buttons[BTN::X])
			{
				float newIncrease = editSettings->heightIncrease - gt.DeltaTime() * (editSettings->heightIncreaseMax / 4.0f);
				editSettings->heightIncrease = MathHelper::clampH(newIncrease, editSettings->heightIncreaseMin,
																 editSettings->heightIncreaseMax);
			}

		}

		/*control for paint tool*/
		else if (editSettings->toolMode == EditTool::Paint)
		{

			if (inputData.Pressed(BTN::Y))
			{
				editSettings->usedTextureIndex = (editSettings->usedTextureIndex + 1) % editSettings->textureMax;
			}

			if (inputData.current.trigger[TRG::RIGHT_TRIGGER] > 0.15f)
			{
				activeLevel->mTerrain->paint(editSettings->Position.x,
											editSettings->Position.y,
											editSettings->FallOffRadius,
											editSettings->BaseRadius,
											gt.DeltaTime() * editSettings->paintIncrease * 10.0f * inputData.current.trigger[TRG::RIGHT_TRIGGER],
											editSettings->usedTextureIndex);
			}

			if (inputData.current.trigger[TRG::LEFT_TRIGGER] > 0.15f)
			{
				activeLevel->mTerrain->paint(editSettings->Position.x,
											 editSettings->Position.y,
											 editSettings->FallOffRadius,
											 editSettings->BaseRadius,
											 gt.DeltaTime() * -editSettings->paintIncrease * 10.0f * inputData.current.trigger[TRG::LEFT_TRIGGER],
											 editSettings->usedTextureIndex);
			}

			/*control increase value*/
			if (inputData.current.buttons[BTN::B])
			{
				float newIncrease = editSettings->paintIncrease + gt.DeltaTime() * (editSettings->paintIncreaseMax);
				editSettings->paintIncrease = MathHelper::clampH(newIncrease, editSettings->paintIncreaseMin,
																  editSettings->paintIncreaseMax);
			}
			else if (inputData.current.buttons[BTN::X])
			{
				float newIncrease = editSettings->paintIncrease - gt.DeltaTime() * (editSettings->paintIncreaseMax);
				editSettings->paintIncrease = MathHelper::clampH(newIncrease, editSettings->paintIncreaseMin,
																  editSettings->paintIncreaseMax);
			}

		}



		/*common camera update*/
		float zoomDelta = inputData.current.trigger[TRG::THUMB_RY] * -25.0f * gt.DeltaTime();

		XMFLOAT3 newCamTarget = XMFLOAT3(editSettings->Position.x,
										 activeLevel->mTerrain->getHeight(editSettings->Position.x, editSettings->Position.y),
										 editSettings->Position.y
										 );

		editCamera->updateFixedCamera(newCamTarget, 
								      zoomDelta
									  );

		/*move light*/
		XMFLOAT3 newLightPos = newCamTarget;
		newLightPos.y += editSettings->BaseRadius / 2.0f;

		editLight->setPosition(newLightPos);
		editLight->setFallOffStart(editSettings->FallOffRadius);
		editLight->setFallOffEnd(editSettings->BaseRadius);
	}
	else
	{
		if (inputData.Pressed(BTN::A))
		{
			ServiceProvider::getAudio()->add(ServiceProvider::getAudioGuid(), "action");
		}

		/*fps camera controls*/
		if (inputData.Released(BTN::DPAD_RIGHT))
		{
			fpsCameraMode = !fpsCameraMode;

			if (fpsCameraMode)
			{
				ServiceProvider::setActiveCamera(fpsCamera);
			}
			else
			{
				ServiceProvider::setActiveCamera(activeLevel->mCameras[0]);
			}
		}

		if (fpsCameraMode)
		{
			fpsCamera->updateFPSCamera(inputData.current, gt);
		}

	}

	/*debug actions*/
	if (inputData.Released(BTN::DPAD_DOWN) && settingsData->miscSettings.DebugEnabled)
	{
		renderResource->toggleHitBoxDraw();
	}

	if (inputData.Released(BTN::DPAD_UP) && settingsData->miscSettings.DebugEnabled)
	{
		ServiceProvider::getSettings()->graphicSettings.SobelFilter = !ServiceProvider::getSettings()->graphicSettings.SobelFilter;
	}

	if (inputData.Released(BTN::DPAD_LEFT) && settingsData->miscSettings.DebugEnabled)
	{
		ServiceProvider::getSettings()->miscSettings.DebugQuadEnabled = !ServiceProvider::getSettings()->miscSettings.DebugQuadEnabled;
	}

	activeLevel->update(gt);
	ServiceProvider::getActiveCamera()->updateViewMatrix();
	renderResource->updateBuffers(gt);

	/*save input for next frame*/
	ServiceProvider::getInputManager()->setPrevious(inputData.current);
	ServiceProvider::getInputManager()->releaseInput();
}


/*=====================*/
/*  Draw  */
/*=====================*/
void P_4E53::draw(const GameTime& gt)
{
	
	auto renderResource = ServiceProvider::getRenderResource();
	auto mCurrentFrameResource = renderResource->getCurrentFrameResource();

	auto cmdListAlloc = mCurrentFrameResource->CmdListAlloc;
	ThrowIfFailed(cmdListAlloc->Reset());

	mCommandList->Reset(
		cmdListAlloc.Get(),
		renderResource->getPSO(RenderType::ShadowDefault)
	);


	ID3D12DescriptorHeap* descriptorHeaps[] = { renderResource->mSrvDescriptorHeap.Get() };
	mCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	mCommandList->SetGraphicsRootSignature(renderResource->mMainRootSignature.Get());

	auto matBuffer = mCurrentFrameResource->MaterialBuffer->getResource();
	mCommandList->SetGraphicsRootShaderResourceView(2, matBuffer->GetGPUVirtualAddress());
	mCommandList->SetGraphicsRootDescriptorTable(5, renderResource->mSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());

	/*draw shadows if enabled*/
	if (ServiceProvider::getSettings()->graphicSettings.ShadowEnabled)
	{
		mCommandList->SetGraphicsRootDescriptorTable(3, renderResource->mNullSrv);

		CD3DX12_GPU_DESCRIPTOR_HANDLE nullCube = renderResource->mNullSrv;
		nullCube.Offset(1, ServiceProvider::getRenderResource()->mCbvSrvUavDescriptorSize);

		mCommandList->SetGraphicsRootDescriptorTable(4, nullCube);

		drawToShadowMap();
	}

	/****/
	mCommandList->RSSetViewports(1, &mScreenViewport);
	mCommandList->RSSetScissorRects(1, &mScissorRect);

	auto offscreenRT = renderResource->getRenderTarget();

	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(offscreenRT->getResource(),
								  D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_RENDER_TARGET));

	/*clear buffers*/
	mCommandList->ClearRenderTargetView(offscreenRT->getRtv(), Colors::LimeGreen, 0, nullptr);
	mCommandList->ClearDepthStencilView(getDepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

	/*set render target*/
	mCommandList->OMSetRenderTargets(1, &offscreenRT->getRtv(), true, &getDepthStencilView());

	auto pass2CB = mCurrentFrameResource->PassCB->getResource();
	mCommandList->SetGraphicsRootConstantBufferView(1, pass2CB->GetGPUVirtualAddress());



	/*bind shadow map to slot 3 and cubemap to 4*/
	if (ServiceProvider::getSettings()->graphicSettings.ShadowEnabled)
	{
		mCommandList->SetGraphicsRootDescriptorTable(3, renderResource->getShadowMap()->Srv());
	}
	else
	{
		CD3DX12_GPU_DESCRIPTOR_HANDLE whiteDescriptor(ServiceProvider::getRenderResource()->mSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
		whiteDescriptor.Offset(ServiceProvider::getRenderResource()->mTextures["white.dds"]->index, ServiceProvider::getRenderResource()->mCbvSrvUavDescriptorSize);
		mCommandList->SetGraphicsRootDescriptorTable(3, whiteDescriptor);
	}

	/*draw terrain*/
	mCommandList->SetGraphicsRootSignature(renderResource->mTerrainRootSignature.Get());
	renderResource->setPSO(ServiceProvider::getEditSettings()->WireFrameOn ? RenderType::TerrainWireFrame : RenderType::Terrain);

	mCommandList->SetGraphicsRootDescriptorTable(4, ServiceProvider::getActiveLevel()->mTerrain->blendTexturesHandle[0]);
	mCommandList->SetGraphicsRootDescriptorTable(5, ServiceProvider::getActiveLevel()->mTerrain->blendTexturesHandle[1]);
	mCommandList->SetGraphicsRootDescriptorTable(6, ServiceProvider::getActiveLevel()->mTerrain->blendTexturesHandle[2]);
	mCommandList->SetGraphicsRootDescriptorTable(7, ServiceProvider::getActiveLevel()->mTerrain->blendTexturesHandle[3]);

	ServiceProvider::getActiveLevel()->drawTerrain();

	mCommandList->SetGraphicsRootSignature(renderResource->mMainRootSignature.Get());
	mCommandList->SetGraphicsRootDescriptorTable(4, ServiceProvider::getActiveLevel()->defaultCubeMapHandle);
	mCommandList->SetGraphicsRootDescriptorTable(5, renderResource->mSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());

	/*draw everything*/

	ServiceProvider::getActiveLevel()->draw();

	/*to srv read*/
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(offscreenRT->getResource(),
								  D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_GENERIC_READ));

	renderResource->getSobelFilter()->execute(mCommandList.Get(),
											  renderResource->mPostProcessRootSignature.Get(),
											  renderResource->getPSO(RenderType::Sobel),
											  offscreenRT->getSrv());

	// Indicate a state transition on the resource usage.
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(getCurrentBackBuffer(),
								  D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	// Specify the buffers we are going to render to.
	mCommandList->OMSetRenderTargets(1, &getCurrentBackBufferView(), true, &getDepthStencilView());

	mCommandList->SetGraphicsRootSignature(renderResource->mPostProcessRootSignature.Get());
	mCommandList->SetPipelineState(renderResource->getPSO(RenderType::Composite));
	mCommandList->SetGraphicsRootDescriptorTable(0, offscreenRT->getSrv());

	if (ServiceProvider::getSettings()->graphicSettings.SobelFilter)
	{
		mCommandList->SetGraphicsRootDescriptorTable(1, renderResource->getSobelFilter()->getOutputSrv());
	}
	else
	{
		CD3DX12_GPU_DESCRIPTOR_HANDLE whiteDescriptor(ServiceProvider::getRenderResource()->mSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
		whiteDescriptor.Offset(ServiceProvider::getRenderResource()->mTextures["white.dds"]->index, ServiceProvider::getRenderResource()->mCbvSrvUavDescriptorSize);
		mCommandList->SetGraphicsRootDescriptorTable(1, whiteDescriptor);
	}
	

	mCommandList->IASetVertexBuffers(0, 1, nullptr);
	mCommandList->IASetIndexBuffer(nullptr);
	mCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	mCommandList->DrawInstanced(6, 1, 0, 0);

	editModeHUD->draw();

	// Indicate a state transition on the resource usage.
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
	renderResource->getCurrentFrameResource()->Fence = ++mCurrentFence;
	mCommandQueue->Signal(mFence.Get(), mCurrentFence);

}

void P_4E53::drawToShadowMap()
{
	auto renderResource = ServiceProvider::getRenderResource();

	mCommandList->RSSetViewports(1, &renderResource->getShadowMap()->Viewport());
	mCommandList->RSSetScissorRects(1, &renderResource->getShadowMap()->ScissorRect());

	// Change to DEPTH_WRITE.
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(renderResource->getShadowMap()->Resource(),
								  D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_DEPTH_WRITE));

	UINT passCBByteSize = d3dUtil::CalcConstantBufferSize(sizeof(PassConstants));

	// Clear the back buffer and depth buffer.
	mCommandList->ClearDepthStencilView(renderResource->getShadowMap()->Dsv(),
										D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

	// Set null render target because we are only going to draw to
	// depth buffer.  Setting a null render target will disable color writes.
	// Note the active PSO also must specify a render target count of 0.
	mCommandList->OMSetRenderTargets(0, nullptr, false, &renderResource->getShadowMap()->Dsv());

	// Bind the pass constant buffer for the shadow map pass.
	auto passCB = renderResource->getCurrentFrameResource()->PassCB->getResource();
	D3D12_GPU_VIRTUAL_ADDRESS passCBAddress = passCB->GetGPUVirtualAddress() + 1 * passCBByteSize;
	mCommandList->SetGraphicsRootConstantBufferView(1, passCBAddress);

	ServiceProvider::getActiveLevel()->drawShadow();

	// Change back to GENERIC_READ so we can read the texture in a shader.
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(renderResource->getShadowMap()->Resource(),
								  D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_GENERIC_READ));

}
