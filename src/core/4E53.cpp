#include <Windows.h>
#include "dx12app.h"
#include "../util/settings.h"
#include "../util/serviceprovider.h"

#define SETTINGS_FILE "cfg/settings.xml"

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

	int vsyncIntervall;

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
	srand(time(NULL));

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
}

bool P_4E53::Initialize()
{
	if (!DX12App::Initialize())
		return false;

	/**copy settings **/
	vsyncIntervall = ServiceProvider::getSettings()->displaySettings.VSync;


	/*initialize input manager*/
	std::shared_ptr<InputManager> inputManager(new InputManager());
	ServiceProvider::setInputManager(inputManager);

	inputThread = std::make_unique<std::thread>(&InputManager::Loop, inputManager);

	/*initialize audio engine*/
	std::shared_ptr<SoundEngine> soundEngine(new SoundEngine());
	ServiceProvider::setAudioEngine(soundEngine);

	soundEngine->init();

	/*load all sound files*/
	soundEngine->loadFile(L"data\\sound\\action.wav", SoundType::Effect);

	/*start the audio thread*/
	audioThread = std::make_unique<std::thread>(&SoundEngine::run, soundEngine);


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
	/*get input*/
	InputSet& inputData = ServiceProvider::getInputManager()->getInput();



	if (inputData.Pressed(BUTTON_A))
	{
		ServiceProvider::getAudio()->add(rand() % 100 +2, "action");
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
	// Reuse the memory associated with command recording.
	// We can only reset when the associated command lists have finished execution on the GPU.
	ThrowIfFailed(mDirectCmdListAlloc->Reset());

	// A command list can be reset after it has been added to the command queue via ExecuteCommandList.
	// Reusing the command list reuses memory.
	ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));

	// Indicate a state transition on the resource usage.
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(getCurrentBackBuffer(),
								  D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	// Set the viewport and scissor rect.  This needs to be reset whenever the command list is reset.
	mCommandList->RSSetViewports(1, &mScreenViewport);
	mCommandList->RSSetScissorRects(1, &mScissorRect);

	// Clear the back buffer and depth buffer.
	mCommandList->ClearRenderTargetView(getCurrentBackBufferView(), Colors::LimeGreen, 0, nullptr);
	mCommandList->ClearDepthStencilView(getDepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

	// Specify the buffers we are going to render to.
	mCommandList->OMSetRenderTargets(1, &getCurrentBackBufferView(), true, &getDepthStencilView());

	// Indicate a state transition on the resource usage.
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(getCurrentBackBuffer(),
								  D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

	// Done recording commands.
	ThrowIfFailed(mCommandList->Close());

	// Add the command list to the queue for execution.
	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	// swap the back and front buffers
	ThrowIfFailed(mSwapChain->Present(vsyncIntervall, 0));
	mCurrBackBuffer = (mCurrBackBuffer + 1) % SwapChainBufferCount;

	// Wait until frame commands are complete.  This waiting is inefficient and is
	// done for simplicity.  Later we will show how to organize our rendering code
	// so we do not have to wait per frame.
	flushCommandQueue();
}
