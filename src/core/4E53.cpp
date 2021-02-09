#include <Windows.h>
#include "dx12app.h"
#include "../util/serviceprovider.h"
#include "../audio/soundengine.h"
#include "../render/renderresource.h"
#include "../core/camera.h"
#include "../core/fpscamera.h"
#include "../core/fixedcamera.h"
#include "../core/level.h"
#include "../core/player.h"
#include "../hud/editmodehud.h"
#include "../util/collisiondatabase.h"
#include "../util/debuginfo.h"
#include "../physics/bulletphysics.h"
#include "../maze/maze.h"
#include "../core/title.h"
#include "../core/transition.h"
#include "../core/coins.h"
#include <filesystem>

#ifndef _DEBUG
inline const std::string SETTINGS_FILE = "config/settings.json";
#else
inline const std::string SETTINGS_FILE = "config/dbg.json";
#endif

using namespace DirectX;

class P_4E53 : public DX12App
{
public:
    P_4E53(HINSTANCE hInstance);
    ~P_4E53();

    virtual bool Initialize()override final;

private:

    virtual void createRtvAndDsvDescriptorHeaps()override final;
    virtual void onResize()override final;
    virtual void update(const GameTime& gt)override final;
    virtual void draw(const GameTime& gt)override final;

    UINT getPointLightIndex(const std::string& name, UINT direction)const;

    int vsyncIntervall = 0;

    BulletPhysics physics;
    CollisionDatabase collisionData;
    TitleItems titleSelection = TitleItems::NewGame;
    Transition mTransition{};

    std::unique_ptr<std::thread> inputThread;
    std::unique_ptr<std::thread> audioThread;

    std::unique_ptr<EditModeHUD> editModeHUD = nullptr;

    std::shared_ptr<Player> mPlayer;

    std::shared_ptr<FPSCamera> fpsCamera;
    std::shared_ptr<FixedCamera> mainCamera;
    std::shared_ptr<Camera> titleCamera;

    bool fpsCameraMode = false;

    std::shared_ptr<FixedCamera> editCamera;
    std::shared_ptr<FixedCamera> editLightCamera;
    LightObject* editLight = nullptr;

    std::vector<std::shared_ptr<Level>> mLevel;

    std::vector<std::string> mPointLightNames;

    bool mToNewGame = false;

    DirectX::BoundingBox goalBox{};

    void setupNewMaze();
    void drawFrameStats();
    void drawToShadowMap();
    void setModelSelection();
    void resetCollisionOnModelSwitch();

    float roundTime = 0.0f;
    const XMFLOAT3 titlePlayerPos = { -11.7801f,1.0f,-62.9093f };
    const XMFLOAT3 titlePlayerRot = { 0.0f,-0.307f, 0.0f };
    const std::string titlePlayerAnimation = "geo_Stretch_01";
    unsigned int bgmID = -1;

};

int gNumFrameResources = 1;

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);


/*******/
/*main entry point*/
/*******/

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE /*hPrevInstance*/,
                   _In_ LPSTR /*lpCmdLine*/, _In_ int /*nCmdShow*/)
{
#ifdef _DEBUG
    //_CrtSetDbgFlag(_CrtSetDbgFlag(0) | _CRTDBG_CHECK_ALWAYS_DF);
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
    _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_DEBUG);
#endif
    
    auto startTime = std::chrono::system_clock::now();

    int status = 0;

    /*create logger*/
    std::shared_ptr<Logger<LogPolicy>> vsLogger(new Logger<LogPolicy>(L""));
    vsLogger->setThreadName("mainThread");
    ServiceProvider::setLoggingService(vsLogger);

    ServiceProvider::getLogger()->print<Severity::Info>("Logger started successfully.");


    /*rand*/
    auto randomizer = std::make_shared<Randomizer>();
    ServiceProvider::setRandomizer(randomizer);

    /*load settings file*/
    SettingsLoader settingsLoader;

    if (!settingsLoader.loadSettings(SETTINGS_FILE))
    {
        ServiceProvider::getLogger()->print<Severity::Error>("Failed to load settings.xml!");
        MessageBox(nullptr, L"Failed to load settings.xml!", L"Error", MB_OK);
        return -1;
    }
    ServiceProvider::setSettings(settingsLoader.get());

    /*create console output window*/
    if(settingsLoader.get()->miscSettings.DebugEnabled)
    {
        AllocConsole();
        FILE* dummy = freopen("CONOUT$", "w", stdout);
    }

    ServiceProvider::getLogger()->print<Severity::Info>("Settings file loaded successfully.");

    /*maze generator*/
    auto mazeGen = std::make_shared<Maze>(ServiceProvider::getSettings()->gameplaySettings.RandomSeed);
    ServiceProvider::setMaze(mazeGen);

    if(ServiceProvider::getSettings()->gameplaySettings.MazeAlgorithm >= static_cast<int>(MazeAlgorithm::Count))
    {
        ServiceProvider::getSettings()->gameplaySettings.MazeAlgorithm = 0;
    }

    mazeGen->algorithm = static_cast<MazeAlgorithm>(ServiceProvider::getSettings()->gameplaySettings.MazeAlgorithm);
    mazeGen->setBraidRatio(ServiceProvider::getSettings()->gameplaySettings.MazeBraidRatio);

    /*misc output*/
    LOG(Severity::Info, "Debug Mode is " << (ServiceProvider::getSettings()->miscSettings.DebugEnabled ? "enabled" : "disabled") << ".");


    if (ServiceProvider::getSettings()->miscSettings.EditModeEnabled)
    {
        ServiceProvider::setGameState(GameState::EDITOR);
    }
    else
    {
        ServiceProvider::setGameState(GameState::TITLE);
    }
    
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
    catch (DxException& e)
    {
        LOG(Severity::Error, "Exception thrown: " << e.toString().c_str());
        MessageBox(nullptr, e.toString().c_str(), L"HR Failed", MB_OK);

        status = -1;

        return status;
    }
}

/*class members*/

P_4E53::P_4E53(HINSTANCE hInstance)
    : DX12App(hInstance)
{

    mWindowCaption = L"Amaze";
}

P_4E53::~P_4E53()
{
    /*clean up*/

    ServiceProvider::getInputManager()->Stop();
    inputThread->join();

    ServiceProvider::getAudio()->forceStop(bgmID);
    ServiceProvider::getAudio()->Stop();
    audioThread->join();

    ServiceProvider::getAudio()->uninit();
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

    /*register bullet physics*/
    ServiceProvider::setPhysics(&physics);
    ServiceProvider::setCollisionDatabase(&collisionData);

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
    std::filesystem::path skinnedPath(SKINNED_PATH);
    std::filesystem::path animPath(ANIM_PATH);

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

    if (!renderResource->init(mDevice.Get(), mCommandList.Get(), texturePath, modelPath, skinnedPath, animPath))
    {
        ServiceProvider::getLogger()->print<Severity::Error>("Initialising render resouce failed!");
        return false;
    }
    renderResource->cmdQueue = mCommandQueue.Get();

    ServiceProvider::setRenderResource(renderResource);

    //setup dear imgui
    ImGui_ImplDX12_Init(mDevice.Get(), gNumFrameResources, DXGI_FORMAT_R8G8B8A8_UNORM,
                        renderResource->mSrvDescriptorHeap.Get(),
                        renderResource->mSrvDescriptorHeap.Get()->GetCPUDescriptorHandleForHeapStart(),
                        renderResource->mSrvDescriptorHeap.Get()->GetGPUDescriptorHandleForHeapStart());


    /*load collision data base from and file and overwrite the base extracted from the models*/
    collisionData.load();


    /*load first level*/
    const std::string suffix = ".level";
    std::string levelFile = "basemaze";

    auto level = std::make_shared<Level>();

    if (__argc == 2)
    {
        levelFile = __argv[1];
    
        // cut off ".level" if it's there
        if(levelFile.size() >= suffix.size() && 0 == levelFile.compare(levelFile.size() - suffix.size(), suffix.size(), suffix))
        {
            levelFile = levelFile.substr(0, levelFile.size() - suffix.size());
        }
    }

    levelFile += suffix;

    if (!Level::levelExists(levelFile))
    {
        LOG(Severity::Info, levelFile << " not found, creating new..");
        if (!level->createNew(levelFile))
        {
            LOG(Severity::Error, "Failed to create new level!");
            return 0;
        }
    }

    if (!level->load(levelFile))
    {
        return 0;
    }

    mLevel.push_back(std::move(level));
    ServiceProvider::setActiveLevel(mLevel.back());

    ServiceProvider::getActiveLevel()->setupMazeGrid(
                    ServiceProvider::getMaze()->getGrid().columns(),
                    ServiceProvider::getMaze()->getGrid().rows() 
                                                );

    /*initialize player and camera*/

    if(!ServiceProvider::getSettings()->miscSettings.EditModeEnabled)
    {
        mainCamera = std::make_shared<FixedCamera>();
        mainCamera->initFixedDistance(10.0f, 15.0f);

        titleCamera = std::make_shared<FixedCamera>();

        //Pos: -5.42424 | 8.21301 | -77.1866 
        // Look: -0.457481 | -0.368652 | 0.809201 
        // Up: -0.181437 | 0.929568 | 0.320913
        titleCamera->lookAt(XMFLOAT3{ -5.42424f,8.21301f,-77.1866f },
                            XMFLOAT3{ -5.42424f - 0.457481f,8.21301f -0.368652f, -77.1866f + 0.809201 },
                            XMFLOAT3{ 0,1,0});

        mPlayer = std::make_shared<Player>("geo");

        mPlayer->setPosition(titlePlayerPos);
        mPlayer->setRotation(titlePlayerRot);

        ServiceProvider::getPhysics()->addCharacter(*mPlayer);
        mPlayer->setupController();
        ServiceProvider::getPhysics()->addAction(mPlayer->getController());

        ServiceProvider::setActiveCamera(titleCamera);
        ServiceProvider::setPlayer(mPlayer);
        
        mPlayer->getController()->resetMovement();
        mPlayer->setAnimation(SP_ANIM(titlePlayerAnimation));
    }
    else
    {
        editModeHUD = std::make_unique<EditModeHUD>();
        editModeHUD->init();
    }

    /*init fpscamera*/
    fpsCamera = std::make_shared<FPSCamera>();
    fpsCamera->setLens();
    fpsCamera->setPosition(0.0f, 5.0f, -20.f);

    /*init edit mode camera if needed*/
    if (ServiceProvider::getSettings()->miscSettings.EditModeEnabled)
    {
        editCamera = std::make_shared<FixedCamera>();
        editCamera->initFixedDistance(2.0f, 80.0f);
        editCamera->setLens();
        editCamera->updateFixedCamera(XMFLOAT3(0.0f, 0.0f, 0.0f), 0.0f, 0.0f);

        ServiceProvider::setActiveCamera(editCamera);

        editLightCamera = std::make_shared<FixedCamera>();
        editLightCamera->initFixedDistance(2.0f, 80.0f);
        editLightCamera->setLens();
        editLightCamera->updateFixedCamera(XMFLOAT3(0.0f, 0.0f, 0.0f), 5.0f, 0.0f);


        for (int i = 0; i < ServiceProvider::getActiveLevel()->mLightObjects.size(); i++)
        {
            if (ServiceProvider::getActiveLevel()->mLightObjects[i]->name == "EditLight")
            {
                editLight = ServiceProvider::getActiveLevel()->mLightObjects[i].get();
                break;
            }

            
        }

        if (!editLight)
        {
            LOG(Severity::Warning, "Could not find Edit Light in level!");
            editLight = ServiceProvider::getActiveLevel()->mLightObjects[3].get();
        }

        
        editLight->setStrength(XMFLOAT3(1.0f, 1.0f, 1.0f));


        /*fill point light names*/
        UINT counter = 0;
        bool firstFound = false;
        for (const auto& l : ServiceProvider::getActiveLevel()->mLightObjects)
        {
            if (l->getLightType() == LightType::Point)
            {
                if(l->name != "EditLight")
                    mPointLightNames.push_back(l->name);

                if (l->name != "EditLight" && !firstFound)
                {
                    firstFound = true;
                    ServiceProvider::getEditSettings()->currentLightSelectionPointIndex = counter;
                    ServiceProvider::getEditSettings()->currentLightSelectionIndex = counter;
                }
            }

            counter++;
        }

        if (ServiceProvider::getActiveLevel()->mGameObjects.size() > 0)
        {
            std::vector<GameObject*> validGameObjects;

            for (const auto& g : ServiceProvider::getActiveLevel()->mGameObjects)
            {
                if ((g.second->gameObjectType == ObjectType::Default ||
                    g.second->gameObjectType == ObjectType::Wall ||
                    g.second->gameObjectType == ObjectType::Skinned) &&
                    g.second->isSelectable)
                {
                    validGameObjects.push_back(g.second.get());
                }
            }

            ServiceProvider::getEditSettings()->currentSelectionIndex = 0;
            ServiceProvider::getEditSettings()->currentSelection = validGameObjects[ServiceProvider::getEditSettings()->currentSelectionIndex];

        }

        /*init ordered Models*/
        for (const auto& e : renderResource->mModels)
        {
            ServiceProvider::getEditSettings()->orderedModels[e.second->group].push_back(e.second.get());
        }

        for (auto& e : ServiceProvider::getEditSettings()->orderedModels)
        {
            std::sort(e.second.begin(), e.second.end(),
                      [&](const Model* a, const Model* b) -> bool
                      {
                          return a->name < b->name;
                      });
        }

        setModelSelection();

    }

    // Execute the initialization commands.
    ThrowIfFailed(mCommandList->Close());
    ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
    mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

    // Wait until initialization is complete.
    flushCommandQueue();


    //transition in on game start
    mTransition.start();

    bgmID = ServiceProvider::getAudioGuid();
    ServiceProvider::getAudio()->add(bgmID, "bgm");

    return true;
}

void P_4E53::createRtvAndDsvDescriptorHeaps()
{
    D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc{};
    rtvHeapDesc.NumDescriptors = SwapChainBufferCount + 1; /*+ 1 off screen render target*/
    rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    rtvHeapDesc.NodeMask = 0;
    ThrowIfFailed(mDevice->CreateDescriptorHeap(
        &rtvHeapDesc, IID_PPV_ARGS(mRtvHeap.GetAddressOf())));

    D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc{};
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

    if (renderResource->getBlurFilter() != nullptr)
    {
        renderResource->getBlurFilter()->onResize(ServiceProvider::getSettings()->displaySettings.ResolutionWidth,
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
    ServiceProvider::updateInput();
    InputSet inputData = ServiceProvider::getInput();
    Settings* settingsData = ServiceProvider::getSettings();

    activeCamera->mPreviousPosition = activeCamera->getPosition3f();


    /***********************/

    //input hold time test
    //if(inputData.getButtonHoldTime(BTN::B) > 10.0f)
    //{
    //    LOG(Severity::Debug, "B");
    //}

    //if(inputData.getTriggerHoldTime(TRG::RIGHT_TRIGGER) > 5.0f)
    //{
    //    LOG(Severity::Debug, "TRG");
    //}

    /************************/
    /**** Edit Mode *********/
    /************************/
    if (ServiceProvider::getGameState() == GameState::EDITOR)
    {
        auto editSettings = ServiceProvider::getEditSettings();

        /*save map*/
        if (inputData.Released(BTN::START))
        {
            editSettings->saveSuccess = activeLevel->save();
            editSettings->savedAnim = 2.0f;
        }

        editSettings->savedAnim -= gt.DeltaTime();

        if (editSettings->savedAnim < 0.0f) editSettings->savedAnim = 0.0f;

        /*switch tool*/
        if (inputData.Pressed(BTN::RIGHT_SHOULDER))
        {
            if (editSettings->toolMode == EditTool::Height)
            {
                editSettings->toolMode = EditTool::Paint;
            }
            else if (editSettings->toolMode == EditTool::Paint)
            {
                editSettings->toolMode = EditTool::Light;
                ServiceProvider::setActiveCamera(editLightCamera);
            }
            else if (editSettings->toolMode == EditTool::Light)
            {
                editSettings->toolMode = EditTool::ObjectTransform;
                ServiceProvider::setActiveCamera(editCamera);
            }
            else if (editSettings->toolMode == EditTool::ObjectTransform)
            {
                editSettings->toolMode = EditTool::ObjectMeta;
            }
            else if (editSettings->toolMode == EditTool::ObjectMeta)
            {
                editSettings->toolMode = EditTool::ObjectCollision;
            }
            else if(editSettings->toolMode == EditTool::ObjectCollision)
            {
                editSettings->toolMode = EditTool::Height;
            }

        }

        else if (inputData.Pressed(BTN::LEFT_SHOULDER))
        {
            if (editSettings->toolMode == EditTool::Height)
            {
                editSettings->toolMode = EditTool::ObjectCollision;
            }
            else if (editSettings->toolMode == EditTool::Paint)
            {
                editSettings->toolMode = EditTool::Height;
            }
            else if (editSettings->toolMode == EditTool::Light)
            {
                editSettings->toolMode = EditTool::Paint;
                ServiceProvider::setActiveCamera(editCamera);
            }
            else if (editSettings->toolMode == EditTool::ObjectTransform)
            {
                editSettings->toolMode = EditTool::Light;
                ServiceProvider::setActiveCamera(editLightCamera);
            }
            else if (editSettings->toolMode == EditTool::ObjectMeta)
            {
                editSettings->toolMode = EditTool::ObjectTransform;
            }
            else if(editSettings->toolMode == EditTool::ObjectCollision)
            {
                editSettings->toolMode = EditTool::ObjectMeta;
            }

        }

        else if (inputData.Pressed(BTN::DPAD_UP))
        {
            if (editSettings->toolMode == EditTool::Camera)
            {
                editSettings->toolMode = editSettings->prevTool;
                if (editSettings->toolMode == EditTool::Light)
                {
                    ServiceProvider::setActiveCamera(editLightCamera);
                }
                else
                {
                    ServiceProvider::setActiveCamera(editCamera);
                }
                
            }
            else
            {
                editSettings->prevTool = editSettings->toolMode;
                editSettings->toolMode = EditTool::Camera;
                ServiceProvider::setActiveCamera(fpsCamera);
            }
        }

        /*edit selection update*/

        if (editSettings->toolMode == EditTool::Height ||
            editSettings->toolMode == EditTool::Paint)
        {
            editSettings->Velocity = editSettings->BaseVelocity * editCamera->getDistanceNormalized();


            XMFLOAT2 in = XMFLOAT2(inputData.current.trigger[TRG::THUMB_LX], inputData.current.trigger[TRG::THUMB_LY]);
            XMFLOAT2 v{};

            v.x = in.x * std::cosf(-editCamera->getTurn()) - in.y * std::sinf(-editCamera->getTurn());
            v.y = in.x * std::sinf(-editCamera->getTurn()) + in.y * std::cosf(-editCamera->getTurn());

            editSettings->Position.x -= v.x * editSettings->Velocity * gt.DeltaTime();
            editSettings->Position.y -= v.y * editSettings->Velocity * gt.DeltaTime();

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

            editSettings->BaseRadius = editCamera->getDistanceNormalized() * editSettings->BaseSelectSize;
            if (editSettings->BaseRadius < 1.0f) editSettings->BaseRadius = 1.0f;
            editSettings->FallOffRadius = editSettings->BaseRadius * editSettings->FallOffRatio;

        }

        /*control for height tool*/
        if (editSettings->toolMode == EditTool::Height)
        {
            /*reset height save*/
            if (inputData.Pressed(BTN::Y))
            {
                editSettings->resetHeight = activeLevel->mTerrain->getHeight(editSettings->Position.x,
                                                                             editSettings->Position.y);
            }

            /*generate random height map*/
            if (inputData.Pressed(BTN::BACK))
            {
                activeLevel->mTerrain->generateHeight();
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
                                                      inputData.current.buttons[BTN::A],
                                                      0.0f/*inputData.current.buttons[BTN::X]*/);
            }

            if (inputData.current.trigger[TRG::LEFT_TRIGGER] > 0.15f)
            {
                activeLevel->mTerrain->increaseHeight(editSettings->Position.x,
                                                      editSettings->Position.y,
                                                      editSettings->FallOffRadius,
                                                      editSettings->BaseRadius,
                                                      gt.DeltaTime() * -editSettings->heightIncrease * inputData.current.trigger[TRG::LEFT_TRIGGER],
                                                      editSettings->resetHeight,
                                                      inputData.current.buttons[BTN::A],
                                                      0.0f/*inputData.current.buttons[BTN::X]*/);
            }

            /*control increase value*/
            if (inputData.current.buttons[BTN::B])
            {
                float newIncrease = editSettings->heightIncrease + gt.DeltaTime() * (editSettings->heightIncreaseMax / 1.25f);
                editSettings->heightIncrease = MathHelper::clampH(newIncrease, editSettings->heightIncreaseMin,
                                                                  editSettings->heightIncreaseMax);
            }
            else if (inputData.current.buttons[BTN::X])
            {
                float newIncrease = editSettings->heightIncrease - gt.DeltaTime() * (editSettings->heightIncreaseMax / 1.25f);
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

        /*object tools*/
        else if (editSettings->toolMode == EditTool::ObjectTransform || editSettings->toolMode == EditTool::ObjectMeta)
        {

            if (editSettings->toolMode == EditTool::ObjectTransform)
            {
                /*switch object transform tool*/
                if (inputData.Pressed(BTN::B))
                {
                    editSettings->objTransformTool = static_cast<ObjectTransformTool>(((int)editSettings->objTransformTool + 1) % 3);
                }

                if (inputData.Pressed(BTN::A))
                {
                    if (editSettings->objTransformTool == ObjectTransformTool::Translation)
                    {
                        editSettings->translationAxis = static_cast<TranslationAxis>(((int)editSettings->translationAxis + 1) % 5);
                    }
                    else if (editSettings->objTransformTool == ObjectTransformTool::Scale)
                    {
                        editSettings->scaleAxis = static_cast<ScaleAxis>(((int)editSettings->scaleAxis + 1) % 4);
                    }
                    else if (editSettings->objTransformTool == ObjectTransformTool::Rotation)
                    {
                        editSettings->rotationAxis = static_cast<RotationAxis>(((int)editSettings->rotationAxis + 1) % 3);
                    }
                }

                /*reset to zero*/
                if (inputData.Pressed(BTN::X))
                {
                    if (editSettings->objTransformTool == ObjectTransformTool::Translation)
                    {
                        XMFLOAT3 nPos = editSettings->currentSelection->getPosition();
                        switch (editSettings->translationAxis)
                        {
                            case TranslationAxis::XY: nPos.x = 0.0f; 
                                                      nPos.y = activeLevel->mTerrain->getHeight(nPos.x, nPos.z) +
                                                                editSettings->currentSelection->getCollider().getFrustumBox().Extents.y;
                                                      break;
                            case TranslationAxis::XZ: nPos.x = 0.0f; nPos.z = 0.0f; break;
                            case TranslationAxis::X: nPos.x = 0.0f; break;
                            case TranslationAxis::Y: nPos.y = activeLevel->mTerrain->getHeight(nPos.x, nPos.z) +
                                                                editSettings->currentSelection->getCollider().getFrustumBox().Extents.y;
                                                    break;
                            case TranslationAxis::Z: nPos.z = 0.0f; break;
                        }

                        editSettings->currentSelection->setPosition(nPos);
                    }
                    else if (editSettings->objTransformTool == ObjectTransformTool::Scale)
                    {
                        editSettings->currentSelection->setScale({ 1.0f,1.0f,1.0f });
                    }
                    else if (editSettings->objTransformTool == ObjectTransformTool::Rotation)
                    {
                        XMFLOAT3 nRotation = editSettings->currentSelection->getRotation();
                        switch (editSettings->rotationAxis)
                        {
                            case RotationAxis::X: nRotation.x = 0.0f; break;
                            case RotationAxis::Y: nRotation.y = 0.0f; break;
                            case RotationAxis::Z: nRotation.z = 0.0f; break;
                        }

                        editSettings->currentSelection->setRotation(nRotation);
                    }
                }

                /*rotation next 45 degrees*/
                if (inputData.Pressed(BTN::Y) && editSettings->objTransformTool == ObjectTransformTool::Rotation)
                {
                    XMFLOAT3 nRotation = editSettings->currentSelection->getRotation();

                    float rot{};
                    switch (editSettings->rotationAxis)
                    {
                        case RotationAxis::X: rot = nRotation.x; break;
                        case RotationAxis::Y: rot = nRotation.y; break;
                        case RotationAxis::Z: rot = nRotation.z; break;
                    }

                    int counter = 0;
                    for (int i = 0; i < 16; i++)
                    {
                        counter++;

                        if (rot < ((counter - 1) * (XM_PIDIV4 / 2)))
                        {
                            break;
                        }
                    }

                    rot = counter == 16 ? 0.0f : counter * (XM_PIDIV4 / 2);

                    switch (editSettings->rotationAxis)
                    {
                        case RotationAxis::X: nRotation.x = rot; break;
                        case RotationAxis::Y: nRotation.y = rot; break;
                        case RotationAxis::Z: nRotation.z = rot;  break;
                    }

                    editSettings->currentSelection->setRotation(nRotation);
                }

                /*translation tool*/
                XMFLOAT3 nPos = editSettings->currentSelection->getPosition();

                float camDistance = editCamera->getDistanceNormalized();

                float thumbX = editSettings->translationIncreaseBase * camDistance * inputData.current.trigger[TRG::THUMB_LX] * gt.DeltaTime();
                float thumbY = editSettings->translationIncreaseBase * camDistance * inputData.current.trigger[TRG::THUMB_LY] * gt.DeltaTime();

                switch (editSettings->translationAxis)
                {
                    case TranslationAxis::XY: nPos.x += thumbX; nPos.y += thumbY; break;
                    case TranslationAxis::XZ: nPos.x += thumbX; nPos.z += thumbY; break;
                    case TranslationAxis::X: nPos.x += thumbX * 0.2f; break;
                    case TranslationAxis::Y: nPos.y += thumbY * 0.2f; break;
                    case TranslationAxis::Z: nPos.z += thumbY * 0.2f; break;
                }

                editSettings->currentSelection->setPosition(nPos);

                /*scale and rotation tool*/
                if (inputData.current.trigger[TRG::RIGHT_TRIGGER] > 0.15f || inputData.current.trigger[TRG::LEFT_TRIGGER])
                {
                    /*which trigger pressed more*/

                    float trigger = inputData.current.trigger[TRG::LEFT_TRIGGER] > inputData.current.trigger[TRG::RIGHT_TRIGGER] ?
                        -inputData.current.trigger[TRG::LEFT_TRIGGER] : inputData.current.trigger[TRG::RIGHT_TRIGGER];

                    /*scale*/
                    if (editSettings->objTransformTool == ObjectTransformTool::Scale)
                    {
                        XMFLOAT3 nScale = editSettings->currentSelection->getScale();

                        float increase = editSettings->scaleIncreaseBase * trigger * gt.DeltaTime();

                        switch (editSettings->scaleAxis)
                        {
                            case ScaleAxis::XYZ:
                                nScale.x += increase;
                                nScale.y += increase;
                                nScale.z += increase;
                                break;
                            case ScaleAxis::X: nScale.x += increase; break;
                            case ScaleAxis::Y: nScale.y += increase; break;
                            case ScaleAxis::Z: nScale.z += increase; break;
                        }

                        editSettings->currentSelection->setScale(nScale);
                        

                    /*rotation*/
                    }
                    else if (editSettings->objTransformTool == ObjectTransformTool::Rotation)
                    {
                        XMFLOAT3 nRotation = editSettings->currentSelection->getRotation();

                        float increase = editSettings->scaleIncreaseBase * trigger * gt.DeltaTime();

                        switch (editSettings->rotationAxis)
                        {
                            case RotationAxis::X: nRotation.x += increase; if (nRotation.x > XM_2PI) nRotation.x -= XM_2PI; break;
                            case RotationAxis::Y: nRotation.y += increase; if (nRotation.y > XM_2PI) nRotation.y -= XM_2PI; break;
                            case RotationAxis::Z: nRotation.z += increase; if (nRotation.z > XM_2PI) nRotation.z -= XM_2PI; break;
                        }

                        editSettings->currentSelection->setRotation(nRotation);
                    }
                }
            }

            /****Object Meta Mode*/
            /*********************/
            else if (editSettings->toolMode == EditTool::ObjectMeta)
            {

                /*switch to invisible wall*/
                if (inputData.Pressed(BTN::LEFT_THUMB))
                {
                    if (editSettings->currentSelection->gameObjectType == ObjectType::Default)
                    {
                        editSettings->currentSelection->renderItem->staticModel = renderResource->mModels["box"].get();
                        editSettings->currentSelection->renderItem->MaterialOverwrite = renderResource->mMaterials["invWall"].get();
                        editSettings->currentSelection->gameObjectType = ObjectType::Wall;
                        editSettings->currentSelection->renderItem->renderType = RenderType::DefaultTransparency;
                        editSettings->currentSelection->renderItem->shadowType = ShadowRenderType::ShadowAlpha;
                        editSettings->currentSelection->renderItem->NumFramesDirty = gNumFrameResources;
                        editSettings->currentSelection->isDrawEnabled = false;
                        editSettings->currentSelection->isShadowEnabled = false;
                        editSettings->currentSelection->isShadowForced = false;
                        editSettings->currentSelection->isCollisionEnabled = true;

                        editSettings->currentSelection->getCollider().setBaseBoxes(renderResource->mModels["box"]->baseModelBox);
                        editSettings->currentSelection->updateTransforms();
                        activeLevel->calculateRenderOrderSizes();

                        setModelSelection();
                        resetCollisionOnModelSwitch();
                    }

                }

                /*switch model group*/
                if (inputData.Pressed(BTN::B) && editSettings->currentSelection->gameObjectType == ObjectType::Default)
                {

                    for (auto it = editSettings->orderedModels.begin();
                         it != editSettings->orderedModels.end();
                         it++)
                    {
                        if ((*it).first == editSettings->selectedGroup)
                        {

                            if ( std::next(it) == editSettings->orderedModels.end())
                            {
                                editSettings->selectedGroup = editSettings->orderedModels.begin()->first;
                            }
                            else
                            {
                                editSettings->selectedGroup = (++it)->first;
                            }
                            break;
                        }
                    }

                    editSettings->currentSelection->renderItem->staticModel = editSettings->orderedModels[editSettings->selectedGroup][0];

                    if (editSettings->selectedGroup == "default")
                    {
                        editSettings->currentSelection->renderItem->MaterialOverwrite = renderResource->mMaterials["default"].get();
                    }
                    else
                    {
                        editSettings->currentSelection->renderItem->MaterialOverwrite = nullptr;
                    }

                    editSettings->currentSelection->setTextureScale(XMFLOAT3(1.0f, 1.0f, 1.0f));
                    editSettings->currentSelection->renderItem->NumFramesDirty = gNumFrameResources;

                    editSettings->currentSelection->getCollider().setBaseBoxes(editSettings->currentSelection->renderItem->staticModel->baseModelBox);
                    editSettings->currentSelection->updateTransforms();
                    setModelSelection();

                    resetCollisionOnModelSwitch();
                }

                if (inputData.Pressed(BTN::X) && editSettings->currentSelection->gameObjectType == ObjectType::Default)
                {

                    for (auto it = editSettings->orderedModels.begin();
                         it != editSettings->orderedModels.end();
                         it++)
                    {
                        if ((*it).first == editSettings->selectedGroup)
                        {

                            if (it == editSettings->orderedModels.begin())
                            {
                                editSettings->selectedGroup = (--editSettings->orderedModels.end())->first;
                            }
                            else
                            {
                                editSettings->selectedGroup = (--it)->first;
                            }
                            break;
                        }
                    }

                    editSettings->currentSelection->renderItem->staticModel = editSettings->orderedModels[editSettings->selectedGroup][0];

                    if (editSettings->selectedGroup == "default")
                    {
                        editSettings->currentSelection->renderItem->MaterialOverwrite = renderResource->mMaterials["default"].get();
                    }
                    else
                    {
                        editSettings->currentSelection->renderItem->MaterialOverwrite = nullptr;
                    }

                    editSettings->currentSelection->setTextureScale(XMFLOAT3(1.0f, 1.0f, 1.0f));
                    editSettings->currentSelection->renderItem->NumFramesDirty = gNumFrameResources;
                    editSettings->currentSelection->getCollider().setBaseBoxes(editSettings->currentSelection->renderItem->staticModel->baseModelBox);
                    editSettings->currentSelection->updateTransforms();
                    setModelSelection();

                    resetCollisionOnModelSwitch();
                }


                /*switch model*/
                else if (inputData.Pressed(BTN::DPAD_RIGHT) && editSettings->currentSelection->gameObjectType == ObjectType::Default)
                {
                    for (auto it = editSettings->orderedModels[editSettings->selectedGroup].begin();
                         it != editSettings->orderedModels[editSettings->selectedGroup].end();
                         it++)
                    {
                        if ( *it == *(editSettings->selectedModel))
                        {

                            if (std::next(it) != editSettings->orderedModels[editSettings->selectedGroup].end())
                            {
                                editSettings->selectedModel = std::next(it);
                            }
                            else
                            {
                                editSettings->selectedModel = editSettings->orderedModels[editSettings->selectedGroup].begin();
                            }

                            break;
                        }
                    }

                    editSettings->currentSelection->renderItem->staticModel = *(editSettings->selectedModel);


                    if (editSettings->selectedGroup == "default")
                    {
                        editSettings->currentSelection->renderItem->MaterialOverwrite = renderResource->mMaterials["default"].get();
                    }
                    else
                    {
                        editSettings->currentSelection->renderItem->MaterialOverwrite = nullptr;
                    }

                    editSettings->currentSelection->getCollider().setBaseBoxes(editSettings->currentSelection->renderItem->staticModel->baseModelBox);
                    editSettings->currentSelection->updateTransforms();
                    editSettings->currentSelection->setTextureScale(XMFLOAT3(1.0f, 1.0f, 1.0f));
                    editSettings->currentSelection->renderItem->NumFramesDirty = gNumFrameResources;

                    resetCollisionOnModelSwitch();
                }


                else if (inputData.Pressed(BTN::DPAD_LEFT) && editSettings->currentSelection->gameObjectType == ObjectType::Default)
                {
                    for (auto it = editSettings->orderedModels[editSettings->selectedGroup].begin();
                         it != editSettings->orderedModels[editSettings->selectedGroup].end();
                         it++)
                    {
                        if (*it == *(editSettings->selectedModel))
                        {

                            if (it != editSettings->orderedModels[editSettings->selectedGroup].begin())
                            {
                                editSettings->selectedModel = std::prev(it);
                            }
                            else
                            {
                                editSettings->selectedModel = std::prev(editSettings->orderedModels[editSettings->selectedGroup].end());
                            }

                            break;
                        }
                    }

                    editSettings->currentSelection->renderItem->staticModel = *(editSettings->selectedModel);


                    if (editSettings->selectedGroup == "default")
                    {
                        editSettings->currentSelection->renderItem->MaterialOverwrite = renderResource->mMaterials["default"].get();
                    }
                    else
                    {
                        editSettings->currentSelection->renderItem->MaterialOverwrite = nullptr;
                    }

                    editSettings->currentSelection->getCollider().setBaseBoxes(editSettings->currentSelection->renderItem->staticModel->baseModelBox);
                    editSettings->currentSelection->updateTransforms();
                    editSettings->currentSelection->setTextureScale(XMFLOAT3(1.0f, 1.0f, 1.0f));
                    editSettings->currentSelection->renderItem->NumFramesDirty = gNumFrameResources;

                    resetCollisionOnModelSwitch();
                }



                /*switch render type*/
                else if (inputData.Pressed(BTN::RIGHT_THUMB) && editSettings->currentSelection->gameObjectType == ObjectType::Default)
                {
                    switch (editSettings->currentSelection->renderItem->renderType)
                    {
                        case RenderType::Default:	editSettings->currentSelection->renderItem->renderType = RenderType::DefaultAlpha;
                            editSettings->currentSelection->renderItem->shadowType = ShadowRenderType::ShadowAlpha;
                            break;
                        case RenderType::DefaultAlpha:	editSettings->currentSelection->renderItem->renderType = RenderType::DefaultTransparency;
                            editSettings->currentSelection->renderItem->shadowType = ShadowRenderType::ShadowAlpha;
                            break;
                        case RenderType::DefaultTransparency:	editSettings->currentSelection->renderItem->renderType = RenderType::DefaultNoNormal;
                            editSettings->currentSelection->renderItem->shadowType = ShadowRenderType::ShadowDefault;
                            break;
                        case RenderType::DefaultNoNormal:	editSettings->currentSelection->renderItem->renderType = RenderType::NoCullNoNormal;
                            editSettings->currentSelection->renderItem->shadowType = ShadowRenderType::ShadowAlpha;
                            break;
                        case RenderType::NoCullNoNormal: editSettings->currentSelection->renderItem->renderType = RenderType::Default;
                            editSettings->currentSelection->renderItem->shadowType = ShadowRenderType::ShadowDefault;
                            break;
                        default: break;
                    }
                    activeLevel->calculateRenderOrderSizes();
                }

                /*copy to new object*/
                else if (inputData.Pressed(BTN::Y) && 
                        (editSettings->currentSelection->gameObjectType == ObjectType::Default ||
                         editSettings->currentSelection->gameObjectType == ObjectType::Water ||
                         editSettings->currentSelection->gameObjectType == ObjectType::Wall))
                {
                    auto prevGO = editSettings->currentSelection;

                    json newGO = editSettings->currentSelection->toJson();

                    std::string baseName = newGO["Name"];
                    baseName = baseName.substr(0, baseName.find_last_of("_"));

                    UINT counter = 1;
                    while (activeLevel->mGameObjects.find(newGO["Name"]) != activeLevel->mGameObjects.end())
                    {
                        newGO["Name"] = baseName + "_(" + std::to_string(counter) + ")";
                        counter++;
                    }

                    activeLevel->addGameObject(newGO);

                    LOG(Severity::Info, "Create new game object " << newGO["Name"] << ".");

                    for (const auto& e : activeLevel->mGameObjects)
                    {
                        if (e.second->Name == newGO["Name"])
                        {
                            editSettings->currentSelection = e.second.get();


                            /*find out the selection index*/
                            std::vector<GameObject*> validGameObjects;

                            for (const auto& g : activeLevel->mGameObjects)
                            {
                                if (g.second->gameObjectType == ObjectType::Default ||
                                    g.second->gameObjectType == ObjectType::Wall ||
                                    g.second->gameObjectType == ObjectType::Skinned || 
                                    g.second->gameObjectType == ObjectType::Water)
                                {
                                    validGameObjects.push_back(g.second.get());
                                }
                            }

                            UINT icounter = 0;
                            for (const auto elem : validGameObjects)
                            {
                                if (elem == editSettings->currentSelection)
                                {
                                    break;
                                }
                                icounter++;
                            }

                            editSettings->currentSelectionIndex = icounter;

                        }
                    }

                    editSettings->currentSelection->gameObjectType = prevGO->gameObjectType;

                    if (editSettings->currentSelection->gameObjectType == ObjectType::Water)
                    {
                        editSettings->currentSelection->renderItem->renderType = RenderType::Water;
                    }

                    setModelSelection();
                }

                /*game object property*/
                else if (inputData.Pressed(BTN::DPAD_DOWN))
                {
                    
                    switch (editSettings->gameObjectProperty)
                    {
                        case GameObjectProperty::Collision: editSettings->gameObjectProperty = GameObjectProperty::Draw; break;
                        case GameObjectProperty::Draw: editSettings->gameObjectProperty = GameObjectProperty::Shadow; break;
                        case GameObjectProperty::Shadow: editSettings->gameObjectProperty = GameObjectProperty::ShadowForce; break;
                        case GameObjectProperty::ShadowForce: editSettings->gameObjectProperty = GameObjectProperty::Collision; break;
                    }

                }

                else if (inputData.Pressed(BTN::A))
                {

                    switch (editSettings->gameObjectProperty)
                    {
                        case GameObjectProperty::Collision: editSettings->currentSelection->isCollisionEnabled = !editSettings->currentSelection->isCollisionEnabled;  break;
                        case GameObjectProperty::Draw: editSettings->currentSelection->isDrawEnabled = !editSettings->currentSelection->isDrawEnabled;  break;
                        case GameObjectProperty::Shadow: editSettings->currentSelection->isShadowEnabled = !editSettings->currentSelection->isShadowEnabled;  break;
                        case GameObjectProperty::ShadowForce: editSettings->currentSelection->isShadowForced = !editSettings->currentSelection->isShadowForced;  break;
                    }

                }
            }
        }

        /*object collision*/
        else if(editSettings->toolMode == EditTool::ObjectCollision)
        {

            //switch type
            if(inputData.Pressed(BTN::Y))
            {
                // next shape
                switch(editSettings->currentSelection->getShape())
                {
                    case BOX_SHAPE_PROXYTYPE: editSettings->currentSelection->setShape(SPHERE_SHAPE_PROXYTYPE); break;
                    case SPHERE_SHAPE_PROXYTYPE: editSettings->currentSelection->setShape(CYLINDER_SHAPE_PROXYTYPE); break;
                    case CYLINDER_SHAPE_PROXYTYPE: editSettings->currentSelection->setShape(CAPSULE_SHAPE_PROXYTYPE); break;
                    case CAPSULE_SHAPE_PROXYTYPE: editSettings->currentSelection->setShape(BOX_SHAPE_PROXYTYPE); break;
                }

                editSettings->collisionScaleAxis = ScaleAxis::XYZ;
            }

            //switch axis
            if(inputData.Pressed(BTN::A))
            {
                editSettings->collisionScaleAxis = static_cast<ScaleAxis>(((int)editSettings->collisionScaleAxis + 1) % 4);

                switch(editSettings->currentSelection->getShape())
                {
                    case BOX_SHAPE_PROXYTYPE: break;
                    case SPHERE_SHAPE_PROXYTYPE: editSettings->collisionScaleAxis = ScaleAxis::XYZ; break;
                    case CAPSULE_SHAPE_PROXYTYPE: if(editSettings->collisionScaleAxis == ScaleAxis::Z) editSettings->collisionScaleAxis = ScaleAxis::XYZ; break;
                    case CYLINDER_SHAPE_PROXYTYPE: if(editSettings->collisionScaleAxis == ScaleAxis::Z) editSettings->collisionScaleAxis = ScaleAxis::XYZ; break;
                }

            }

            /*scale tool*/
            if(inputData.current.trigger[TRG::RIGHT_TRIGGER] > 0.15f || inputData.current.trigger[TRG::LEFT_TRIGGER])
            {
                /*which trigger pressed more*/

                float trigger = inputData.current.trigger[TRG::LEFT_TRIGGER] > inputData.current.trigger[TRG::RIGHT_TRIGGER] ?
                    -inputData.current.trigger[TRG::LEFT_TRIGGER] : inputData.current.trigger[TRG::RIGHT_TRIGGER];

                /*scale*/
                XMFLOAT3 nScale = editSettings->currentSelection->extents;

                float increase = editSettings->scaleIncreaseBase * trigger * gt.DeltaTime();

                switch(editSettings->collisionScaleAxis)
                {
                    case ScaleAxis::XYZ:
                        nScale.x += increase;
                        nScale.y += increase;
                        nScale.z += increase;
                        break;
                    case ScaleAxis::X: nScale.x += increase; break;
                    case ScaleAxis::Y: nScale.y += increase; break;
                    case ScaleAxis::Z: nScale.z += increase; break;
                }

                editSettings->currentSelection->extents = nScale;
          
            }

            /*save current object to collision database*/
            if(inputData.Pressed(BTN::RIGHT_THUMB))
            {
                XMFLOAT3 t = editSettings->currentSelection->extents;
                const auto& sRef = editSettings->currentSelection->getScale();
                XMStoreFloat3(&t, XMVectorDivide(XMLoadFloat3(&t), XMLoadFloat3(&sRef)));

                LOG(Severity::Info, "Adding collision of " << editSettings->currentSelection->renderItem->staticModel->name << " to database.");

                collisionData.add(editSettings->currentSelection->renderItem->staticModel->name,
                                  editSettings->currentSelection->getShape(),
                                  t);

                collisionData.save();
            }

            /*switch between physic sliders*/
            if(inputData.Pressed(BTN::X))
            {
                editSettings->selectedPhysicProperty = static_cast<PhysicProperty>((static_cast<int>(editSettings->selectedPhysicProperty) + 1) % 4);
            }

            /*physic property values*/

            if(inputData.current.trigger[TRG::THUMB_LY] != 0.0f)
            {
                const float dir = inputData.current.trigger[TRG::THUMB_LY];
                float value = 0.0f;

                switch(editSettings->selectedPhysicProperty)
                {
                    case PhysicProperty::Mass: 
                        value = editSettings->currentSelection->mass;
                        value += dir * gt.DeltaTime() * 5.0f;
                        editSettings->currentSelection->mass = MathHelper::clampH(value, 0.0f, 1000.0f);

                        if(editSettings->currentSelection->mass > 0.0f)
                        {
                            editSettings->currentSelection->motionType = ObjectMotionType::Dynamic;
                        }
                        else if(editSettings->currentSelection->mass == 0.0f)
                        {
                            editSettings->currentSelection->motionType = ObjectMotionType::Static;
                        }

                        break;
                    case PhysicProperty::Friction: 
                        value = editSettings->currentSelection->friction;
                        value += dir * gt.DeltaTime() * 0.5f;
                        editSettings->currentSelection->friction = MathHelper::clampH(value, 0.0f, 1.0f);
                        break;
                    case PhysicProperty::Restitution:
                        value = editSettings->currentSelection->restitution;
                        value += dir * gt.DeltaTime() * 0.5f;
                        editSettings->currentSelection->restitution = MathHelper::clampH(value, 0.0f, 1.0f);
                        break;
                    case PhysicProperty::Damping: 
                        value = editSettings->currentSelection->damping;
                        value += dir * gt.DeltaTime() * 0.5f;
                        editSettings->currentSelection->damping = MathHelper::clampH(value, 0.0f, 1.0f);
                        break;
                }

            }

        }

        /*light*/
        else if (editSettings->toolMode == EditTool::Light)
        {
            /*switch between light type*/
            if (inputData.Pressed(BTN::BACK))
            {
                editSettings->lightTypeChoice = editSettings->lightTypeChoice == LightTypeChoice::Directional ? LightTypeChoice::Point : LightTypeChoice::Directional;
                editSettings->currentLightSelectionIndex = editSettings->lightTypeChoice == LightTypeChoice::Directional ? editSettings->currentLightSelectionDirectionIndex : editSettings->currentLightSelectionPointIndex;
            }

            /*cycle lights*/
            int selectionDir = inputData.Pressed(BTN::DPAD_LEFT) ? -1 : inputData.Pressed(BTN::DPAD_RIGHT) ? 1 : 0;

            if (selectionDir != 0)
            {
                int index = 0;

                if (editSettings->lightTypeChoice == LightTypeChoice::Directional)
                {
                    index = editSettings->currentLightSelectionDirectionIndex + selectionDir;
                    if (index < 0)
                    {
                        index = 2;
                    }
                    else if (index > 2)
                    {
                        index = 0;
                    }

                    editSettings->currentLightSelectionDirectionIndex = index;
                    editSettings->currentLightSelectionIndex = index;
                }
                else
                {
                    editSettings->currentLightSelectionPointIndex = getPointLightIndex(activeLevel->mLightObjects[editSettings->currentLightSelectionPointIndex]->name, selectionDir);
                    editSettings->currentLightSelectionIndex = editSettings->currentLightSelectionPointIndex;
                }

            }

            /*cycle r g b strength*/
            if (inputData.Pressed(BTN::A))
            {
                switch (editSettings->lightColorAxis)
                {
                    case LightColorAxis::R: editSettings->lightColorAxis = LightColorAxis::G; break;
                    case LightColorAxis::G: editSettings->lightColorAxis = LightColorAxis::B; break;
                    case LightColorAxis::B: editSettings->lightColorAxis = LightColorAxis::R; break;
                }
            }

            /*increase light strength*/
            float rgbIncrease = -inputData.current.trigger[TRG::LEFT_TRIGGER] + inputData.current.trigger[TRG::RIGHT_TRIGGER];

            if (rgbIncrease != 0.f)
            {
                XMFLOAT3 strength = activeLevel->mLightObjects[editSettings->currentLightSelectionIndex]->getStrength();

                rgbIncrease = rgbIncrease * gt.DeltaTime();

                switch (editSettings->lightColorAxis)
                {
                    case LightColorAxis::R: strength.x = MathHelper::clampH(strength.x + rgbIncrease, 0.0f, 1.0f); break;
                    case LightColorAxis::G: strength.y = MathHelper::clampH(strength.y + rgbIncrease, 0.0f, 1.0f); break;
                    case LightColorAxis::B: strength.z = MathHelper::clampH(strength.z + rgbIncrease, 0.0f, 1.0f); break;
                }

                activeLevel->mLightObjects[editSettings->currentLightSelectionIndex]->setStrength(strength);

            }

            /*directional light control*/
            if (editSettings->lightTypeChoice == LightTypeChoice::Directional)
            {
                /*cycle direction x y z*/
                if (inputData.Pressed(BTN::B))
                {
                    switch (editSettings->lightDirectionAxis)
                    {
                        case LightDirectionAxis::X: editSettings->lightDirectionAxis = LightDirectionAxis::Y; break;
                        case LightDirectionAxis::Y: editSettings->lightDirectionAxis = LightDirectionAxis::Z; break;
                        case LightDirectionAxis::Z: editSettings->lightDirectionAxis = LightDirectionAxis::X; break;
                    }
                }

                /*inc / decrease direction*/
                float thumbY = inputData.current.trigger[TRG::THUMB_LY] * 1.0f * gt.DeltaTime();
                XMFLOAT3 dir = activeLevel->mLightObjects[editSettings->currentLightSelectionIndex]->getDirection();

                switch (editSettings->lightDirectionAxis)
                {
                    case LightDirectionAxis::X: dir.x += thumbY; break;
                    case LightDirectionAxis::Y: dir.y += thumbY; break;
                    case LightDirectionAxis::Z: dir.z += thumbY; break;
                }

                XMVECTOR nDir = XMVector3Normalize(XMLoadFloat3(&dir));
                XMStoreFloat3(&dir, nDir);
                activeLevel->mLightObjects[editSettings->currentLightSelectionIndex]->setDirection(dir);

            }
            /*point light control*/
            else
            {
                /*cycle translation xy xz*/
                if (inputData.Pressed(BTN::B))
                {
                    editSettings->lightTranslationAxis = editSettings->lightTranslationAxis == LightTranslationAxis::XY ? LightTranslationAxis::XZ : LightTranslationAxis::XY;
                }

                /*copy to new*/
                if (inputData.Pressed(BTN::Y))
                {
                    json e = activeLevel->mLightObjects[editSettings->currentLightSelectionIndex]->toJson();

                    std::string baseName = e["Name"];
                    baseName = baseName.substr(0, baseName.find_last_of("_"));

                    UINT counter = 1;

                    while (activeLevel->existsLightByName(e["Name"]))
                    {
                        e["Name"] = baseName + "_(" + std::to_string(counter) + ")";
                        counter++;
                    }

                    activeLevel->mLightObjects.insert(activeLevel->mLightObjects.begin() + editSettings->currentLightSelectionIndex, std::make_unique<LightObject>(LightType::Point, e));
                    mPointLightNames.push_back(e["Name"]);

                    LOG(Severity::Info, "Create new point light " << e["Name"] << ".");

                    editSettings->currentLightSelectionIndex = getPointLightIndex(e["Name"], 0);
                    editSettings->currentLightSelectionPointIndex = editSettings->currentLightSelectionIndex;
                }

                /*move light*/
                XMFLOAT3 lPosition = activeLevel->mLightObjects[editSettings->currentLightSelectionIndex]->getPosition();

                float thumbX = inputData.current.trigger[TRG::THUMB_LX] * 10.0f*  gt.DeltaTime();
                float thumbY = inputData.current.trigger[TRG::THUMB_LY] * 10.0f * gt.DeltaTime();

                switch (editSettings->lightTranslationAxis)
                {
                    case LightTranslationAxis::XY: lPosition.x += thumbX; lPosition.y += thumbY; break;
                    case LightTranslationAxis::XZ: lPosition.x += thumbX; lPosition.z += thumbY; break;
                }

                activeLevel->mLightObjects[editSettings->currentLightSelectionIndex]->setPosition(lPosition);


                /*control falloff end / start*/
                float lFoStart = activeLevel->mLightObjects[editSettings->currentLightSelectionIndex]->getFallOffStart();
                float lFoEnd = activeLevel->mLightObjects[editSettings->currentLightSelectionIndex]->getFallOffEnd();

                thumbX = inputData.current.trigger[TRG::THUMB_RX] * 3.0f * gt.DeltaTime();
                thumbY = inputData.current.trigger[TRG::THUMB_RY] * 3.0f * gt.DeltaTime();

                lFoStart += thumbX;
                lFoEnd += thumbY;

                if (lFoStart < activeLevel->mLightObjects[editSettings->currentLightSelectionIndex]->getFallOffEnd())
                {
                    activeLevel->mLightObjects[editSettings->currentLightSelectionIndex]->setFallOffStart(lFoStart);
                }

                if (lFoEnd > activeLevel->mLightObjects[editSettings->currentLightSelectionIndex]->getFallOffStart())
                {
                    activeLevel->mLightObjects[editSettings->currentLightSelectionIndex]->setFallOffEnd(lFoEnd);
                }

            }

        }




        XMFLOAT3 newCamTarget{};
        float zoomDelta = 0.0f;

        /*common camera update for paint & height*/

        if (editSettings->toolMode != EditTool::ObjectTransform &&
            editSettings->toolMode != EditTool::ObjectMeta &&
            editSettings->toolMode != EditTool::Camera &&
            editSettings->toolMode != EditTool::ObjectCollision &&
            editSettings->toolMode != EditTool::Light)
        {
            zoomDelta = inputData.current.trigger[TRG::THUMB_RY] * -50.0f * gt.DeltaTime();

            newCamTarget = XMFLOAT3(editSettings->Position.x,
                                    activeLevel->mTerrain->getHeight(editSettings->Position.x, editSettings->Position.y),
                                    editSettings->Position.y
            );

            /*move light*/
            XMFLOAT3 newLightPos = newCamTarget;
            newLightPos.y += editSettings->BaseRadius / 4.0f;

            editLight->setPosition(newLightPos);
            editLight->setFallOffStart(editSettings->FallOffRadius);
            editLight->setFallOffEnd(editSettings->BaseRadius);
        }

        /*camera update for object mode*/
        else if (editSettings->toolMode == EditTool::ObjectTransform || editSettings->toolMode == EditTool::ObjectMeta)
        {
            zoomDelta = inputData.current.trigger[TRG::THUMB_RY] * -50.0f * gt.DeltaTime();

            if (editSettings->currentSelection != nullptr)
            {
                newCamTarget = editSettings->currentSelection->getPosition();
            }
        }
        else if(editSettings->toolMode == EditTool::ObjectCollision)
        {
            zoomDelta = inputData.current.trigger[TRG::THUMB_RY] * -50.0f * gt.DeltaTime();

            if(editSettings->currentSelection != nullptr)
            {
                newCamTarget = editSettings->currentSelection->getPosition();
            }
        }

        if (editSettings->toolMode != EditTool::Camera && editSettings->toolMode != EditTool::Light)
        {
            
            float turnInput = static_cast<float>(static_cast<int>(inputData.current.buttons[BTN::DPAD_LEFT]) -
                                                 static_cast<int>(inputData.current.buttons[BTN::DPAD_RIGHT]));

            float turnDelta = turnInput * XM_PI * gt.DeltaTime();

            if (editSettings->toolMode == EditTool::ObjectMeta) turnDelta = 0.0f;

            editCamera->updateFixedCamera(newCamTarget,
                                          zoomDelta, turnDelta);
        }
        else if(editSettings->toolMode == EditTool::Camera)
        {
        /*fps camera controls*/

            if (inputData.Pressed(BTN::A))
            {
                float dist = 0.0f;
                float tMin = MathHelper::Infinity;

                GameObject* selectedObject = nullptr;

                for (const auto& e : activeLevel->mGameObjects)
                {
                    if (e.second->gameObjectType == ObjectType::Sky ||
                        e.second->gameObjectType == ObjectType::Terrain ||
                        e.second->gameObjectType == ObjectType::Debug ||
                        !e.second->isSelectable)
                        continue;

                    /*pick object via ray cast from camera*/
                    if (e.second->getCollider().getPickBox().Intersects(fpsCamera->getPosition(),
                        fpsCamera->getLook(),
                        dist))
                    {
                        if (dist < tMin)
                        {
                            tMin = dist;
                            selectedObject = e.second.get();
                        }
                        
                    }
                }

                if (selectedObject)
                {
                    LOG(Severity::Info, "Picked GameObject " << selectedObject->Name << ".");

                    std::vector<GameObject*> validGameObjects;

                    for (const auto& g : activeLevel->mGameObjects)
                    {
                        if (g.second->gameObjectType == ObjectType::Default ||
                            g.second->gameObjectType == ObjectType::Wall ||
                            g.second->gameObjectType == ObjectType::Skinned ||
                            g.second->gameObjectType == ObjectType::Water)
                        {
                            validGameObjects.push_back(g.second.get());
                        }
                    }

                    int index = 0;

                    for (const auto& e : validGameObjects)
                    {
                        if (e == selectedObject)
                        {
                            break;
                        }
                        index++;
                    }

                    editSettings->currentSelection = selectedObject;
                    editSettings->currentSelectionIndex = index;

                    setModelSelection();

                }

            }
            

            /*wireframe on/off*/
            if (inputData.Pressed(BTN::LEFT_THUMB))
            {
                editSettings->WireFrameOn = !editSettings->WireFrameOn;
            }

            /*toggle object property*/
            if (inputData.Pressed(BTN::Y) && editSettings->currentSelection)
            {
                switch (editSettings->gameObjectProperty)
                {
                    case GameObjectProperty::Collision: editSettings->currentSelection->isCollisionEnabled = !editSettings->currentSelection->isCollisionEnabled;  break;
                    case GameObjectProperty::Draw: editSettings->currentSelection->isDrawEnabled = !editSettings->currentSelection->isDrawEnabled;  break;
                    case GameObjectProperty::Shadow: editSettings->currentSelection->isShadowEnabled = !editSettings->currentSelection->isShadowEnabled;  break;
                    case GameObjectProperty::ShadowForce: editSettings->currentSelection->isShadowForced = !editSettings->currentSelection->isShadowForced;  break;
                }
            }

            /*focus on selected object*/
            if (inputData.Pressed(BTN::X))
            {
                if (editSettings->currentSelection)
                {
                    XMFLOAT3 nPos = editSettings->currentSelection->getPosition();
                    auto &box = editSettings->currentSelection->getCollider().getFrustumBox();

                    nPos.z -= MathHelper::maxH(5.0f, box.Extents.z * 4.5f);
                    nPos.y += MathHelper::maxH(4.0f, box.Extents.y * 3.0f);


                    fpsCamera->lookAt(nPos, editSettings->currentSelection->getPosition(), XMFLOAT3(0.0f, 1.0f, 0.0f));
                }

                
            }
            else
            {
                fpsCamera->updateFPSCamera(inputData.current, gt);
            }
            
        }
        else if (editSettings->toolMode == EditTool::Light)
        {
            XMFLOAT3 lPos = activeLevel->mLightObjects[editSettings->currentLightSelectionIndex]->getPosition();

            editLightCamera->updateFixedCamera(lPos, 0.0f, 0.0f);
            

        }

        editModeHUD->update();

    }


    /*************/
    /*Game Update*/
    /*************/
    else if(ServiceProvider::getGameState() == GameState::TITLE)
    {

        /*update player*/
        mPlayer->update(gt);

        //fade out when choosing new game
        if(mToNewGame && !mTransition.inProgress())
        {
            mToNewGame = false;

            //create a new maze
            setupNewMaze();
            mPlayer->getController()->resetMovement();
            mPlayer->getController()->setUnIdle();

            ServiceProvider::setGameState(GameState::INGAME);
            ServiceProvider::setActiveCamera(mainCamera);

            mTransition.start();
            activeLevel->mLightObjects[0]->setDirection(
                { -0.5699999928474426f,
                    -0.5699999928474426f,
                    0.5699999928474426f }
            );
            roundTime = 0.0f;
        }
        else if(!mToNewGame)
        {

            /*selection*/
            if(inputData.Pressed(BTN::DPAD_UP))
            {
                ServiceProvider::getAudio()->add(ServiceProvider::getAudioGuid(), "switch_action");

                switch(titleSelection)
                {
                    case TitleItems::NewGame: titleSelection = TitleItems::Quit; break;
                    case TitleItems::Quit: titleSelection = TitleItems::NewGame; break;
                }

            }
            else if(inputData.Pressed(BTN::DPAD_DOWN))
            {
                ServiceProvider::getAudio()->add(ServiceProvider::getAudioGuid(), "switch_action");

                switch(titleSelection)
                {
                    case TitleItems::NewGame: titleSelection = TitleItems::Quit; break;
                    case TitleItems::Quit: titleSelection = TitleItems::NewGame; break;
                }

            }

            /*action*/
            if(inputData.Pressed(BTN::A))
            {
                if(titleSelection == TitleItems::Quit)
                {
                    LOG(Severity::Info, "Quit selected.");

                    //quits main game loop
                    ServiceProvider::getAudio()->add(ServiceProvider::getAudioGuid(), "confirm_action");
                    mIsRunning = false;
                }
                else if(titleSelection == TitleItems::NewGame)
                {
                    LOG(Severity::Info, "Start game selected.");

                    /*start transition*/
                    mTransition.start();
                    mToNewGame = true;

                    ServiceProvider::getAudio()->add(ServiceProvider::getAudioGuid(), "confirm_action");

                }
            }
        }

    }
    else if(ServiceProvider::getGameState() == GameState::ENDSCREEN)
    {

        /*update player control data*/
        auto controller = mPlayer->getController();

        if(inputData.current.trigger[TRG::RIGHT_TRIGGER] > 0.1f)
        {
            controller->run();
        }

        float inputMagnitude{};
        XMFLOAT2 inputDirection = { inputData.current.trigger[TRG::THUMB_LX], inputData.current.trigger[TRG::THUMB_LY] };
        XMVECTOR inputDirectionV = XMVector2Normalize(XMLoadFloat2(&inputDirection));
        XMVECTOR inputMagnitudeV = XMVector2Length(XMLoadFloat2(&inputDirection));
        XMStoreFloat2(&inputDirection, inputDirectionV);
        XMStoreFloat(&inputMagnitude, inputMagnitudeV);
        inputMagnitude = inputMagnitude > 1.0f ? 1.0f : inputMagnitude;

        if(!fpsCameraMode && roundTime > 0.85f)
        {
            controller->setMovement(inputDirection, inputMagnitude);
        }

        /*update physics simulation*/
        physics.simulateStep(gt.DeltaTime());

        /*update player*/
        mPlayer->update(gt);

        mainCamera->updateFixedCamera(mPlayer->getPosition(), 0.0f, 0.0f);

        //fade out when returning to title menu
        if(mToNewGame && !mTransition.inProgress())
        {
            mToNewGame = false;


            //player to title
            mPlayer->getController()->resetMovement();
            ServiceProvider::getPlayer()->setPosition(titlePlayerPos);
            ServiceProvider::getPlayer()->setRotation(titlePlayerRot);
            mPlayer->setAnimation(SP_ANIM(titlePlayerAnimation));

            ServiceProvider::setGameState(GameState::TITLE);
            ServiceProvider::setActiveCamera(titleCamera);

            mTransition.start();

            roundTime = 0.0f;
            ServiceProvider::getSettings()->gameplaySettings.RandomSeed = -1;
        }
        else if(!mToNewGame)
        {

            //press a to get back to the title menu
            if(inputData.Pressed(BTN::A))
            {
                LOG(Severity::Info, "Return to title menu.");

                /*start transition*/
                mTransition.start();
                mToNewGame = true;

                ServiceProvider::getAudio()->add(ServiceProvider::getAudioGuid(), "confirm_action");
            }
        }
    }
    else if (ServiceProvider::getGameState() == GameState::INGAME)
    {
      
        auto& lDir = activeLevel->mLightObjects[0]->getDirection();
        lDir.x += 0.00025f * gt.DeltaTime();
        XMStoreFloat3(&lDir, XMVector3Normalize(XMLoadFloat3(&lDir)));
        activeLevel->mLightObjects[0]->setDirection(
            lDir
        );

        /*update player control data*/
        auto controller = mPlayer->getController();

        if(inputData.current.trigger[TRG::RIGHT_TRIGGER] > 0.1f)
        {
            controller->run();
        }

        float inputMagnitude{};
        XMFLOAT2 inputDirection = { inputData.current.trigger[TRG::THUMB_LX], inputData.current.trigger[TRG::THUMB_LY] };
        XMVECTOR inputDirectionV = XMVector2Normalize(XMLoadFloat2(&inputDirection));
        XMVECTOR inputMagnitudeV = XMVector2Length(XMLoadFloat2(&inputDirection));
        XMStoreFloat2(&inputDirection, inputDirectionV);
        XMStoreFloat(&inputMagnitude, inputMagnitudeV);
        inputMagnitude = inputMagnitude > 1.0f ? 1.0f : inputMagnitude;

        if(!fpsCameraMode && roundTime > 0.85f)
        {
            controller->setMovement(inputDirection, inputMagnitude);
        }

        /*update physics simulation*/
        physics.simulateStep(gt.DeltaTime());

        /*update player*/
        mPlayer->update(gt);

        /*TODO update objective indicator*/

        //acc time
        roundTime += gt.DeltaTime();

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
                ServiceProvider::setActiveCamera(mainCamera);
            }
        }

        if(inputData.Pressed(BTN::DPAD_LEFT))
        {
            LOG(Severity::Debug, *ServiceProvider::getActiveCamera());
        }


        if (fpsCameraMode)
        {
            fpsCamera->updateFPSCamera(inputData.current, gt);
        }
        else
        {
            mainCamera->updateFixedCamera(mPlayer->getPosition(), 0.0f, 0.0f);
        }

        /*check if the player reached the end*/

        if(goalBox.Contains(BoundingSphere(mPlayer->getPosition(), 0.1f)) ||
           (ServiceProvider::getSettings()->miscSettings.DebugEnabled && inputData.Pressed(BTN::START))) // CHEAT
        {
            ServiceProvider::setGameState(GameState::ENDSCREEN);
            ServiceProvider::getAudio()->add(ServiceProvider::getAudioGuid(), "maze_finished");
        }

        if((ServiceProvider::getSettings()->miscSettings.DebugEnabled && inputData.Pressed(BTN::BACK))) // CHEAT
        {
            for(Coin& c : mPlayer->coins)
            {
                c.collected = true;
            }
        }

    }

    /*debug actions*/
    if (inputData.Released(BTN::DPAD_DOWN) && (settingsData->miscSettings.DebugEnabled && (ServiceProvider::getEditSettings()->toolMode == EditTool::Camera || ServiceProvider::getGameState() == GameState::INGAME)))
    {
        renderResource->toggleRoughHitBoxDraw();
    }


    /*first update camera, then the level*/
    ServiceProvider::getActiveCamera()->updateViewMatrix();

    activeLevel->update(gt);

    renderResource->updateBuffers(gt);

    /*update transition vars*/
    mTransition.update(gt.DeltaTime());

    renderResource->getBlurFilter()->setSigma(mTransition.blur());
    renderResource->setCompositeColor(mTransition.fade());


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
    const auto settings = ServiceProvider::getSettings();

    // Start the Dear ImGui frame
    auto& guiIO = ImGui::GetIO();
    
    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    // Prepare all dear imgui windows

    //ImGui::ShowDemoWindow();

    //draw frame stats overlay
    if(settings->miscSettings.DrawFPSEnabled)
    {
        drawFrameStats();
    }

    float imguiWindowOpacity = (1.0f - mTransition.blurNormalized()) * 0.85f;
    if(imguiWindowOpacity < 0.125f) imguiWindowOpacity = 0.0f;

    // draw temporary title menu
    if(ServiceProvider::getGameState() == GameState::TITLE && imguiWindowOpacity > 0.0f)
    {
        ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings |
            ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoMove;
        ImGui::SetNextWindowBgAlpha(imguiWindowOpacity);


        ImGui::SetNextWindowPos(ImVec2(guiIO.DisplaySize.x * 0.5f, guiIO.DisplaySize.y * 0.5f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));

        ImGui::Begin("Title menu", NULL, windowFlags);
        ImGui::Text(titleSelection == TitleItems::NewGame ? ">START GAME" : "START GAME");
        ImGui::Text(titleSelection == TitleItems::Quit ? ">QUIT" : "QUIT");

        ImGui::End();

        //input seed window
        ImGui::SetNextWindowBgAlpha(imguiWindowOpacity);
        ImGui::SetNextWindowPos(ImVec2(10.0f, guiIO.DisplaySize.y * 0.2f), ImGuiCond_Always);

        ImGui::Begin("input seed", NULL, windowFlags);
        ImGui::Text("Maze Seed (-1 = random):");
        ImGui::InputInt("", &ServiceProvider::getSettings()->gameplaySettings.RandomSeed, 0, 0);
        bool randPressed = ImGui::Button("Randomize Seed");

        if(randPressed)
        {
            ServiceProvider::getSettings()->gameplaySettings.RandomSeed = ServiceProvider::getRandomizer()->nextInt(10000000, 100000);
        }

        ImGui::End();
    }
    else if(ServiceProvider::getGameState() == GameState::INGAME && imguiWindowOpacity > 0.0f)
    {
        {
            ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings |
                ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoMove;
            ImGui::SetNextWindowBgAlpha(imguiWindowOpacity);


            ImGui::SetNextWindowPos(ImVec2(10.0f, guiIO.DisplaySize.y * 0.2f), ImGuiCond_Always);

            ImGui::Begin("ingame", NULL, windowFlags);
            int totalSeconds = static_cast<int>(roundTime);
            int minutes = totalSeconds / 60;
            int seconds = totalSeconds % 60;
            ImGui::Text("Find all coins to unlock the\ndoor at the end of the maze!");
            ImGui::Text("Time: %02d:%02d", minutes, seconds);
            ImGui::Text("Coins: %d/%d", mPlayer->coinCount(), Coins::CoinCount);
            ImGui::End();
        }

    }
    else if(ServiceProvider::getGameState() == GameState::ENDSCREEN && imguiWindowOpacity > 0.0f)
    {
        {
            ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings |
                ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoMove;
            ImGui::SetNextWindowBgAlpha(imguiWindowOpacity);
            
            ImGui::SetNextWindowPos(ImVec2(guiIO.DisplaySize.x * 0.5f, guiIO.DisplaySize.y * 0.5f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));

            ImGui::Begin("end screen", NULL, windowFlags);
            int totalSeconds = static_cast<int>(roundTime);
            int minutes = totalSeconds / 60;
            int seconds = totalSeconds % 60;
            ImGui::Text("You finished the maze! :)");
            ImGui::Text("");
            ImGui::Text("Time: %02d:%02d", minutes, seconds);
            ImGui::Text("Coins: %d/%d", mPlayer->coinCount(), Coins::CoinCount);
            ImGui::Text("");
            ImGui::Text("Press A to continue.");
            ImGui::End();
        }
    }



    /*main dx12 render code*/
    auto cmdListAlloc = mCurrentFrameResource->CmdListAlloc.Get();
    ThrowIfFailed(cmdListAlloc->Reset());

    mCommandList->Reset(
        cmdListAlloc,
        renderResource->getPSO(ShadowRenderType::ShadowDefault)
    );

    ID3D12DescriptorHeap* descriptorHeaps[] = { renderResource->mSrvDescriptorHeap.Get() };
    mCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

    /*set main root signature*/
    mCommandList->SetGraphicsRootSignature(renderResource->mMainRootSignature.Get());

    /*set material buffer and texture array*/
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

    const auto resBarr = CD3DX12_RESOURCE_BARRIER::Transition(offscreenRT->getResource(),
                                                              D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_RENDER_TARGET);
    mCommandList->ResourceBarrier(1, &resBarr);

    /*clear buffers*/
    mCommandList->ClearRenderTargetView(offscreenRT->getRtv(), Colors::LimeGreen, 0, nullptr);
    mCommandList->ClearDepthStencilView(getDepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

    /*set render target*/
    const auto lRtv = offscreenRT->getRtv();
    const auto lDSV = getDepthStencilView();
    mCommandList->OMSetRenderTargets(1, &lRtv, true, &lDSV);

    /*set per pass data*/
    auto pass2CB = mCurrentFrameResource->PassCB->getResource();
    mCommandList->SetGraphicsRootConstantBufferView(1, pass2CB->GetGPUVirtualAddress());

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

    /*switch to terrain signature*/
    mCommandList->SetGraphicsRootSignature(renderResource->mTerrainRootSignature.Get());
    renderResource->setPSO(ServiceProvider::getEditSettings()->WireFrameOn ? RenderType::TerrainWireFrame : RenderType::Terrain);

    /*bind textures directly*/
    mCommandList->SetGraphicsRootDescriptorTable(4, ServiceProvider::getActiveLevel()->mTerrain->blendTexturesHandle[0]);
    mCommandList->SetGraphicsRootDescriptorTable(5, ServiceProvider::getActiveLevel()->mTerrain->blendTexturesHandle[1]);
    mCommandList->SetGraphicsRootDescriptorTable(6, ServiceProvider::getActiveLevel()->mTerrain->blendTexturesHandle[2]);
    mCommandList->SetGraphicsRootDescriptorTable(7, ServiceProvider::getActiveLevel()->mTerrain->blendTexturesHandle[3]);

    ServiceProvider::getActiveLevel()->drawTerrain();

    /*back to main root signature*/
    mCommandList->SetGraphicsRootSignature(renderResource->mMainRootSignature.Get());

    /*bind cube map and texture array*/
    mCommandList->SetGraphicsRootDescriptorTable(4, ServiceProvider::getActiveLevel()->defaultCubeMapHandle);
    mCommandList->SetGraphicsRootDescriptorTable(5, renderResource->mSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());


    if (mPlayer)
    {
        renderResource->setPSO(RenderType::SkinnedDefault);
        mPlayer->draw();
    }

    ServiceProvider::getActiveLevel()->draw();

    /*to srv read*/
    const auto resBarr2 = CD3DX12_RESOURCE_BARRIER::Transition(offscreenRT->getResource(),
                                                               D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_GENERIC_READ);
    mCommandList->ResourceBarrier(1, &resBarr2);

    renderResource->getSobelFilter()->execute(mCommandList.Get(),
                                              renderResource->mPostProcessRootSignature.Get(),
                                              renderResource->getPSO(PostProcessRenderType::Sobel),
                                              offscreenRT->getSrv());

    // Indicate a state transition on the resource usage.
    const auto resBarr3 = CD3DX12_RESOURCE_BARRIER::Transition(getCurrentBackBuffer(),
                                                               D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
    mCommandList->ResourceBarrier(1, &resBarr3);

    // Specify the buffers we are going to render to.
    const auto lCBBV = getCurrentBackBufferView();

    mCommandList->OMSetRenderTargets(1, &lCBBV, true, &lDSV);



    ///*switch to post process root signature*/
    mCommandList->SetGraphicsRootSignature(renderResource->mPostProcessRootSignature.Get());

    if (renderResource->useMultColor)
    {
        mCommandList->SetPipelineState(renderResource->getPSO(PostProcessRenderType::CompositeMult));
    }
    else
    {
        mCommandList->SetPipelineState(renderResource->getPSO(PostProcessRenderType::Composite));
    }
    
    mCommandList->SetGraphicsRoot32BitConstants(0, 4, renderResource->getCompositeColor().data(), 0);
    mCommandList->SetGraphicsRoot32BitConstants(0, 3, renderResource->getMultColor().data(), 4);
    mCommandList->SetGraphicsRootDescriptorTable(1, offscreenRT->getSrv());

    if (ServiceProvider::getSettings()->graphicSettings.SobelFilter)
    {
        mCommandList->SetGraphicsRootDescriptorTable(2, renderResource->getSobelFilter()->getOutputSrv());
    }
    else
    {
        CD3DX12_GPU_DESCRIPTOR_HANDLE whiteDescriptor(ServiceProvider::getRenderResource()->mSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
        whiteDescriptor.Offset(ServiceProvider::getRenderResource()->mTextures["white.dds"]->index, ServiceProvider::getRenderResource()->mCbvSrvUavDescriptorSize);
        mCommandList->SetGraphicsRootDescriptorTable(2, whiteDescriptor);
    }

    mCommandList->IASetVertexBuffers(0, 1, nullptr);
    mCommandList->IASetIndexBuffer(nullptr);
    mCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    mCommandList->DrawInstanced(6, 1, 0, 0);


    /*apply blur filter*/
    auto blurFilter = renderResource->getBlurFilter();

    if (blurFilter->getSigma() > 0.0f)
    {
        blurFilter->execute(mCommandList.Get(), renderResource->mPostProcessRootSignature.Get(),
                            renderResource->getPSO(PostProcessRenderType::BlurHorz), renderResource->getPSO(PostProcessRenderType::BlurVert),
                            getCurrentBackBuffer());

        const auto resBarr = CD3DX12_RESOURCE_BARRIER::Transition(getCurrentBackBuffer(), D3D12_RESOURCE_STATE_COPY_SOURCE,
                                                                  D3D12_RESOURCE_STATE_COPY_DEST);
        const auto resBarr2 = CD3DX12_RESOURCE_BARRIER::Transition(getCurrentBackBuffer(),
                                                                   D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_RENDER_TARGET);

        mCommandList->ResourceBarrier(1, &resBarr);

        mCommandList->CopyResource(getCurrentBackBuffer(), blurFilter->getOutput());

        mCommandList->ResourceBarrier(1, &resBarr2);
    }

    /*draw hud for the edit mode*/

    ImGui::Render();
    if(editModeHUD != nullptr)
    {
        editModeHUD->draw();
    }
    else
    {
        /*draw dear imgui*/
        
        ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), mCommandList.Get());
    }


    const auto resBarr4 = CD3DX12_RESOURCE_BARRIER::Transition(getCurrentBackBuffer(),
                                                               D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
    mCommandList->ResourceBarrier(1, &resBarr4);

    // done
    ThrowIfFailed(mCommandList->Close());

    // add commands to queue
    ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
    mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

    // swap buffers in swapchain
    ThrowIfFailed(mSwapChain->Present(vsyncIntervall, 0));
    mCurrBackBuffer = (mCurrBackBuffer + 1) % SwapChainBufferCount;

    if (editModeHUD != nullptr)
        editModeHUD->commit();

    /*advance fence on gpu to signal that this frame is finished*/
    renderResource->getCurrentFrameResource()->Fence = ++mCurrentFence;
    mCommandQueue->Signal(mFence.Get(), mCurrentFence);
}

UINT P_4E53::getPointLightIndex(const std::string& name, UINT direction) const
{
    UINT counter = 0;
    int ret = 0;

    for (const auto& l : mPointLightNames)
    {
        if (l == name)
        {
            ret = counter;
            break;
        }

        counter++;
    }

    ret += direction;



    if (ret < 0) ret = (int)(mPointLightNames.size()) - 1;
    if (ret > mPointLightNames.size()-1) ret = 0;

    counter = 0;
    for (const auto& l : ServiceProvider::getActiveLevel()->mLightObjects)
    {
        if (l->name == mPointLightNames[ret])
        {
            return counter;
        }
        counter++;
    }

    return 0;
}

void P_4E53::setupNewMaze()
{
    auto& grid = ServiceProvider::getMaze()->getGrid();

    //generate new maze and apply to level
    LOG(Severity::Info, "Creating maze with seed " << ServiceProvider::getSettings()->gameplaySettings.RandomSeed << "...");

    Randomizer rNew{ ServiceProvider::getSettings()->gameplaySettings.RandomSeed };

    ServiceProvider::getMaze()->setRandomizer(rNew);
    ServiceProvider::getMaze()->generate();
    ServiceProvider::getActiveLevel()->updateToGrid(ServiceProvider::getMaze()->getGrid());
    ServiceProvider::getActiveLevel()->resetPhyObjects();

    //set start and goal

    /*start in the middle of the left side*/
    Cell* startCell = grid(0, grid.rows() / 2);
    Distances distances = startCell->distances();

    /*find which cell on the right has the longest way*/

    //int maxLength = 0;
    //int maxIndex = 0;
    //Cell* goalCell = nullptr;

    //for(int i = 0; i < grid.rows(); i++)
    //{
    //    goalCell = grid(grid.columns() - 1, i);

    //    if(distances.get(goalCell) > maxLength)
    //    {
    //        maxLength = distances.get(goalCell);
    //        maxIndex = i;
    //    }

    //}

    //goalCell = grid(grid.columns() - 1, maxIndex);

    /*nope, just take the middle*/
    Cell* goalCell = grid(grid.columns()-1, grid.rows() / 2);


    ServiceProvider::getActiveLevel()->setStartEnd(grid, startCell, goalCell);
    ServiceProvider::getActiveLevel()->setupCoins(grid);

    //position player at the start
    float width = ServiceProvider::getActiveLevel()->mazeBaseWidth;
    float halfWidth = width / 2.0f;
    float plPosX = -width * grid.columns() / 2.0f - width;
    float plPosZ = width * grid.rows() / 2.0f - (grid.rows() / 2) * width - width / 2.0f;

    ServiceProvider::getPlayer()->setPosition({plPosX,8.5f,plPosZ});
    ServiceProvider::getPlayer()->setRotation({ 0.f,0.f,0.f });
    
    //create hitbox at the goal

    float glPosX = width * grid.columns() / 2.0f + width;
    float glPosY = width * grid.rows() / 2.0f - goalCell->getPosition().second * width -width / 2.0f;

    goalBox = BoundingBox({ glPosX, 0.0f, glPosY }, { halfWidth, halfWidth, halfWidth });

    //std::cout << "\n" << glPosX << " , " << glPosY << " " << goalCell->getPosition().second << "\n";


    LOG(Severity::Info, "Generated a maze with algorithm " << static_cast<int>(ServiceProvider::getMaze()->algorithm) << ".");
}

void P_4E53::drawFrameStats()
{
    const float offset = 10.0f;

    const std::vector<float> fpsVec = { ServiceProvider::getDebugInfo()->fpsData.begin(), ServiceProvider::getDebugInfo()->fpsData.end() };
    const float max = fpsVec.empty() ? 0 : *(std::max_element(fpsVec.begin(), fpsVec.end()));


    ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings |
                                    ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoMove;
    ImGui::SetNextWindowBgAlpha(0.5f);

    ImVec2 window_pos = ImVec2(offset, offset);
    ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, { 0,0 });

    ImGui::Begin("Frame stats", NULL, windowFlags);
    ImGui::Text("%.0f FPS | %.3f mspf",
                ServiceProvider::getDebugInfo()->CurrentFPS,
                ServiceProvider::getDebugInfo()->Mspf);
    ImGui::Text("%d object(s) | %d shadow(s)",
                ServiceProvider::getDebugInfo()->DrawnGameObjects,
                ServiceProvider::getDebugInfo()->DrawnShadowObjects);

    ImGui::PlotHistogram("", &fpsVec[0], static_cast<int>(fpsVec.size()), 0, NULL, 0.0f, max, ImVec2(250, 75));
    ImGui::End();

}

void P_4E53::drawToShadowMap()
{
    auto renderResource = ServiceProvider::getRenderResource();

    const auto lSMV = renderResource->getShadowMap()->Viewport();
    const auto lSMSR = renderResource->getShadowMap()->ScissorRect();
    mCommandList->RSSetViewports(1, &lSMV);
    mCommandList->RSSetScissorRects(1, &lSMSR);

    // Change to DEPTH_WRITE.
    const auto resBarr = CD3DX12_RESOURCE_BARRIER::Transition(renderResource->getShadowMap()->Resource(),
                                                              D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_DEPTH_WRITE);
    mCommandList->ResourceBarrier(1, &resBarr);

    UINT passCBByteSize = d3dUtil::CalcConstantBufferSize(sizeof(PassConstants));

    // Clear the back buffer and depth buffer.
    mCommandList->ClearDepthStencilView(renderResource->getShadowMap()->Dsv(),
                                        D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

    // Set null render target because we are only going to draw to
    // depth buffer.  Setting a null render target will disable color writes.
    // Note the active PSO also must specify a render target count of 0.
    const auto lSMDSV = renderResource->getShadowMap()->Dsv();
    mCommandList->OMSetRenderTargets(0, nullptr, false, &lSMDSV);

    // Bind the pass constant buffer for the shadow map pass.
    auto passCB = renderResource->getCurrentFrameResource()->PassCB->getResource();
    D3D12_GPU_VIRTUAL_ADDRESS passCBAddress = passCB->GetGPUVirtualAddress() + 1 * (long long)passCBByteSize;
    mCommandList->SetGraphicsRootConstantBufferView(1, passCBAddress);

    if (mPlayer)
    {
        renderResource->setPSO(ShadowRenderType::ShadowSkinned);
        mPlayer->drawShadow();
    }

    ServiceProvider::getActiveLevel()->drawShadow();

    // Change back to GENERIC_READ so we can read the texture in a shader.
    const auto resBarr2 = CD3DX12_RESOURCE_BARRIER::Transition(renderResource->getShadowMap()->Resource(),
                                                               D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_GENERIC_READ);
    mCommandList->ResourceBarrier(1, &resBarr2);
}

void P_4E53::setModelSelection()
{

    auto editSettings = ServiceProvider::getEditSettings();

    editSettings->selectedGroup = editSettings->currentSelection->renderItem->getModel()->group;

    for (editSettings->selectedModel = editSettings->orderedModels[editSettings->selectedGroup].begin(); 
         editSettings->selectedModel != editSettings->orderedModels[editSettings->selectedGroup].end();
         editSettings->selectedModel++)
    {
        if (*editSettings->selectedModel == editSettings->currentSelection->renderItem->getModel())
        {
            break;
        }
    }

    //LOG(Severity::Debug, editSettings->selectedGroup << " " << (*editSettings->selectedModel)->name);

}

void P_4E53::resetCollisionOnModelSwitch()
{

    auto editSettings = ServiceProvider::getEditSettings();
    const auto& cdb = ServiceProvider::getCollisionDatabase();

    XMFLOAT3 ext = cdb->getExtents(editSettings->currentSelection->renderItem->staticModel->name);
    const XMFLOAT3 lScale = editSettings->currentSelection->getScale();
    XMStoreFloat3(&ext, XMVectorMultiply(XMLoadFloat3(&ext), XMLoadFloat3(&lScale)));

    editSettings->currentSelection->extents = ext;
    editSettings->currentSelection->setShape(cdb->getShapeType(editSettings->currentSelection->renderItem->staticModel->name));
    editSettings->collisionScaleAxis = ScaleAxis::XYZ;

}
