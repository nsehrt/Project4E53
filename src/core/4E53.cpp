#include <Windows.h>
#include "dx12app.h"
#include "../util/settings.h"
#include "../render/frameresource.h"
#include "../util/serviceprovider.h"
#include <filesystem>

#define SETTINGS_FILE "cfg/settings.xml"

#define SOUND_PATH_MUSIC "data/sound/music"
#define SOUND_PATH_EFFECTS "data/sound/effects"

using namespace DirectX;

class P_4E53 : public DX12App
{

public:
	P_4E53(HINSTANCE hInstance);
	~P_4E53();

	virtual bool Initialize()override;

private:
	virtual void onResize()override;
	virtual void update(const GameTime& gt)override;
	virtual void draw(const GameTime& gt)override;

	int vsyncIntervall = 0;

	/*frame resources*/
	int gNumFrameResources = 3;
	std::vector<std::unique_ptr<FrameResource>> mFrameResources;
	FrameResource* mCurrentFrameResource = nullptr;
	int mCurrentFrameResourceIndex = 0;


	UINT mCbvSrvDescriptorSize = 0;


	std::unique_ptr<std::thread> inputThread;
	std::unique_ptr<std::thread> audioThread;
};




/*******/
/*main entry point*/
/*******/

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, 
				   _In_ LPSTR lpCmdLine, _In_ int nCmdShow)
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
	std::shared_ptr<Logger<VSLogPolicy>> vsLogger(new Logger<VSLogPolicy>(L""));
	vsLogger->setThreadName("mainThread");
	ServiceProvider::setVSLoggingService(vsLogger);

	ServiceProvider::getVSLogger()->print<Severity::Info>("Logger started successfully.");


	/*load settings file*/
	SettingsLoader settingsLoader;

	if (!settingsLoader.loadSettings(SETTINGS_FILE))
	{
		ServiceProvider::getVSLogger()->print<Severity::Error>("Failed to load settings.xml!");
		MessageBox(nullptr, L"Failed to load settings.xml!", L"Error", MB_OK);
		return -1;
	}
	ServiceProvider::setSettings(settingsLoader.get());

	ServiceProvider::getVSLogger()->print<Severity::Info>("Settings file loaded successfully.");

	/*initialize main window and directx12*/
	try
	{
		P_4E53 app(hInstance);
		if (!app.Initialize())
		{
			return 0;
		}

		auto endTime = std::chrono::system_clock::now();
		std::chrono::duration<double> elapsedTime = endTime - startTime;
		std::stringstream outStr;
		outStr << "Game initialization was successful. (" << elapsedTime.count() << " seconds)";

		ServiceProvider::getVSLogger()->print<Severity::Info>(outStr.str());

		status = app.run();

		ServiceProvider::getVSLogger()->print<Severity::Info>("Game loop has been quit!");

		return 0;
	}
	catch (DxException & e)
	{
		ServiceProvider::getVSLogger()->print<Severity::Critical>("Exception thrown!");

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

	/*load all sound files*/

	for (const auto& entry : std::filesystem::recursive_directory_iterator(std::filesystem::path(SOUND_PATH_MUSIC)))
	{
		soundEngine->loadFile(entry.path().c_str(), SoundType::Music);
	}

	for (const auto& entry : std::filesystem::recursive_directory_iterator(std::filesystem::path(SOUND_PATH_EFFECTS)))
	{
		soundEngine->loadFile(entry.path().c_str(), SoundType::Effect);
	}

	/*start the audio thread*/
	audioThread = std::make_unique<std::thread>(&SoundEngine::run, soundEngine);


	/*build dx12 resources*/


	/*build frame resources*/
	for (int i = 0; i < gNumFrameResources; i++)
	{
		mFrameResources.push_back(std::make_unique<FrameResource>(mDevice.Get(), 5, 5, 5, 5)); /* TO DO !!!!*/
	}


	// Execute the initialization commands.
	ThrowIfFailed(mCommandList->Close());
	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	// Wait until initialization is complete.
	flushCommandQueue();

	return true;
}

void P_4E53::onResize()
{
	DX12App::onResize();


}


/*=====================*/
/*  Update  */
/*=====================*/
void P_4E53::update(const GameTime& gt)
{
	/******************************/
	/*cycle to next frame resource*/
	mCurrentFrameResourceIndex = (mCurrentFrameResourceIndex + 1) % gNumFrameResources;
	mCurrentFrameResource = mFrameResources[mCurrentFrameResourceIndex].get();

	/*wait for gpu if necessary*/
	if (mCurrentFrameResource->Fence != 0 && mFence->GetCompletedValue() < mCurrentFrameResource->Fence)
	{
		HANDLE eventHandle = CreateEventExW(nullptr, NULL, false, EVENT_ALL_ACCESS);
		ThrowIfFailed(mFence->SetEventOnCompletion(mCurrentFrameResource->Fence, eventHandle));
		WaitForSingleObject(eventHandle, INFINITE);
		CloseHandle(eventHandle);
	}
	/****************************/



	/*get input*/
	InputSet& inputData = ServiceProvider::getInputManager()->getInput();



	if (inputData.Pressed(BUTTON_A))
	{
		ServiceProvider::getAudio()->add(ServiceProvider::getAudioGuid(), "action");
	}




	/*save input for next frame*/
	ServiceProvider::getInputManager()->setPrevious(inputData.current);
	ServiceProvider::getInputManager()->releaseInput();
}


/*=====================*/
/*  Draw  */
/*=====================*/
void P_4E53::draw(const GameTime& gt)
{
	auto cmdListAlloc = mCurrentFrameResource->CmdListAlloc;
	ThrowIfFailed(cmdListAlloc->Reset());

	mCommandList->Reset(
		cmdListAlloc.Get(),
		nullptr
	);

	mCommandList->RSSetViewports(1, &mScreenViewport);
	mCommandList->RSSetScissorRects(1, &mScissorRect);

	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(getCurrentBackBuffer(),
								  D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	/*clear buffers*/
	mCommandList->ClearRenderTargetView(getCurrentBackBufferView(), Colors::LimeGreen, 0, nullptr);
	mCommandList->ClearDepthStencilView(getDepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

	/*set render target*/
	mCommandList->OMSetRenderTargets(1, &getCurrentBackBufferView(), true, &getDepthStencilView());

	/*set used root signature*/
	//mCommandList->SetGraphicsRootSignature();

	/*set per pass constant buffer*/


	/*draw everything*/



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
	mCurrentFrameResource->Fence = ++mCurrentFence;
	mCommandQueue->Signal(mFence.Get(), mCurrentFence);

}
