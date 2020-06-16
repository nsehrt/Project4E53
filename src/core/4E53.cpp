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
#define SETTINGS_FILE "config/settings.xml"
#else
#define SETTINGS_FILE "config/dbg.xml"
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

    UINT getPointLightIndex(const std::string& name, UINT direction)const;

    int vsyncIntervall = 0;

    std::unique_ptr<std::thread> inputThread;
    std::unique_ptr<std::thread> audioThread;

    std::unique_ptr<EditModeHUD> editModeHUD = nullptr;

    std::shared_ptr<FPSCamera> fpsCamera;
    bool fpsCameraMode = false;

    std::shared_ptr<FixedCamera> editCamera;
    std::shared_ptr<FixedCamera> editLightCamera;
    LightObject* editLight = nullptr;

    std::vector<std::shared_ptr<Level>> mLevel;

    std::vector<std::string> mPointLightNames;

    void drawToShadowMap();
    void setModelSelection();

};

int gNumFrameResources = 1;

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

    /*load first level*/
    std::string levelFile = "0";

    auto level = std::make_shared<Level>();

    if (__argc == 2)
    {
        levelFile = __argv[1];
    }

    if (!Level::levelExists(levelFile + ".level"))
    {
        LOG(Severity::Info, levelFile << ".level not found, creating new..");
        if (!level->createNew(levelFile))
        {
            LOG(Severity::Error, "Failed to create new level!");
            return 0;
        }
    }

    levelFile += ".level";

    if (!level->load(levelFile))
    {
        return 0;
    }

    mLevel.push_back(std::move(level));
    ServiceProvider::setActiveLevel(mLevel.back());

    if (ServiceProvider::getSettings()->miscSettings.EditModeEnabled)
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
                if (g.second->gameObjectType == GameObjectType::Static ||
                    g.second->gameObjectType == GameObjectType::Wall ||
                    g.second->gameObjectType == GameObjectType::Dynamic)
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
    InputSet& inputData = ServiceProvider::getInputManager()->getInput();
    Settings* settingsData = ServiceProvider::getSettings();

    activeCamera->mPreviousPosition = activeCamera->getPosition3f();

    /***********************/



    /************************/
    /**** Edit Mode *********/
    /************************/
    if (settingsData->miscSettings.EditModeEnabled)
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
                editSettings->toolMode = EditTool::Height;
                
            }

        }

        else if (inputData.Pressed(BTN::LEFT_SHOULDER))
        {
            if (editSettings->toolMode == EditTool::Height)
            {
                editSettings->toolMode = EditTool::ObjectMeta;
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
            XMFLOAT2 v;

            v.x = in.x * cos(-editCamera->getTurn()) - in.y * sin(-editCamera->getTurn());
            v.y = in.x * sin(-editCamera->getTurn()) + in.y * cos(-editCamera->getTurn());

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
                    editSettings->objTransformTool = (ObjectTransformTool)(((int)editSettings->objTransformTool + 1) % 3);
                }

                if (inputData.Pressed(BTN::A))
                {
                    if (editSettings->objTransformTool == ObjectTransformTool::Translation)
                    {
                        editSettings->translationAxis = (TranslationAxis)(((int)editSettings->translationAxis + 1) % 5);
                    }
                    else if (editSettings->objTransformTool == ObjectTransformTool::Scale)
                    {
                        editSettings->scaleAxis = (ScaleAxis)(((int)editSettings->scaleAxis + 1) % 4);
                    }
                    else if (editSettings->objTransformTool == ObjectTransformTool::Rotation)
                    {
                        editSettings->rotationAxis = (RotationAxis)(((int)editSettings->rotationAxis + 1) % 3);
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
                                                                editSettings->currentSelection->getRoughHitBoxExtentY();
                                                      break;
                            case TranslationAxis::XZ: nPos.x = 0.0f; nPos.z = 0.0f; break;
                            case TranslationAxis::X: nPos.x = 0.0f; break;
                            case TranslationAxis::Y: nPos.y = activeLevel->mTerrain->getHeight(nPos.x, nPos.z) +
                                                                editSettings->currentSelection->getRoughHitBoxExtentY();
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

                    float rot;
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
                    editSettings->currentSelection->renderItem->staticModel = renderResource->mModels["box"].get();
                    editSettings->currentSelection->renderItem->MaterialOverwrite = renderResource->mMaterials["invWall"].get();
                    editSettings->currentSelection->gameObjectType = GameObjectType::Wall;
                    editSettings->currentSelection->renderItem->renderType = RenderType::DefaultTransparency;
                    editSettings->currentSelection->renderItem->shadowType = RenderType::ShadowAlpha;
                    editSettings->currentSelection->renderItem->NumFramesDirty = gNumFrameResources;
                    editSettings->currentSelection->isDrawEnabled = false;
                    editSettings->currentSelection->isShadowEnabled = false;
                    editSettings->currentSelection->isShadowForced = false;
                    editSettings->currentSelection->isCollisionEnabled = true;

                    activeLevel->calculateRenderOrderSizes();

                    setModelSelection();
                }

                /*switch model group*/
                if (inputData.Pressed(BTN::B) && editSettings->currentSelection->gameObjectType == GameObjectType::Static)
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

                    setModelSelection();
                }

                if (inputData.Pressed(BTN::X) && editSettings->currentSelection->gameObjectType == GameObjectType::Static)
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

                    setModelSelection();
                }


                /*switch model*/
                else if (inputData.Pressed(BTN::DPAD_RIGHT) && editSettings->currentSelection->gameObjectType == GameObjectType::Static)
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

                    editSettings->currentSelection->setTextureScale(XMFLOAT3(1.0f, 1.0f, 1.0f));
                    editSettings->currentSelection->renderItem->NumFramesDirty = gNumFrameResources;
                }


                else if (inputData.Pressed(BTN::DPAD_LEFT) && editSettings->currentSelection->gameObjectType == GameObjectType::Static)
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

                    editSettings->currentSelection->setTextureScale(XMFLOAT3(1.0f, 1.0f, 1.0f));
                    editSettings->currentSelection->renderItem->NumFramesDirty = gNumFrameResources;
                }



                /*switch render type*/
                else if (inputData.Pressed(BTN::RIGHT_THUMB) && editSettings->currentSelection->gameObjectType == GameObjectType::Static)
                {
                    switch (editSettings->currentSelection->renderItem->renderType)
                    {
                        case RenderType::Default:	editSettings->currentSelection->renderItem->renderType = RenderType::DefaultAlpha;
                            editSettings->currentSelection->renderItem->shadowType = RenderType::ShadowAlpha;
                            break;
                        case RenderType::DefaultAlpha:	editSettings->currentSelection->renderItem->renderType = RenderType::DefaultTransparency;
                            editSettings->currentSelection->renderItem->shadowType = RenderType::ShadowAlpha;
                            break;
                        case RenderType::DefaultTransparency:	editSettings->currentSelection->renderItem->renderType = RenderType::DefaultNoNormal;
                            editSettings->currentSelection->renderItem->shadowType = RenderType::ShadowDefault;
                            break;
                        case RenderType::DefaultNoNormal:	editSettings->currentSelection->renderItem->renderType = RenderType::NoCullNoNormal;
                            editSettings->currentSelection->renderItem->shadowType = RenderType::ShadowAlpha;
                            break;
                        case RenderType::NoCullNoNormal: editSettings->currentSelection->renderItem->renderType = RenderType::Default;
                            editSettings->currentSelection->renderItem->shadowType = RenderType::ShadowDefault;
                            break;
                    }
                    activeLevel->calculateRenderOrderSizes();
                }

                /*copy to new object*/
                else if (inputData.Pressed(BTN::Y) && 
                        (editSettings->currentSelection->gameObjectType == GameObjectType::Static || 
                         editSettings->currentSelection->gameObjectType == GameObjectType::Water ||
                         editSettings->currentSelection->gameObjectType == GameObjectType::Wall))
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
                        if (e.second->name == newGO["Name"])
                        {
                            editSettings->currentSelection = e.second.get();


                            /*find out the selection index*/
                            std::vector<GameObject*> validGameObjects;

                            for (const auto& g : activeLevel->mGameObjects)
                            {
                                if (g.second->gameObjectType == GameObjectType::Static ||
                                    g.second->gameObjectType == GameObjectType::Wall ||
                                    g.second->gameObjectType == GameObjectType::Dynamic || 
                                    g.second->gameObjectType == GameObjectType::Water)
                                {
                                    validGameObjects.push_back(g.second.get());
                                }
                            }

                            UINT icounter = 0;
                            for (const auto e : validGameObjects)
                            {
                                if (e == editSettings->currentSelection)
                                {
                                    break;
                                }
                                icounter++;
                            }

                            editSettings->currentSelectionIndex = icounter;

                        }
                    }

                    editSettings->currentSelection->gameObjectType = prevGO->gameObjectType;

                    if (editSettings->currentSelection->gameObjectType == GameObjectType::Water)
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




        XMFLOAT3 newCamTarget;
        float zoomDelta = 0.0f;

        /*common camera update for paint & height*/

        if (editSettings->toolMode != EditTool::ObjectTransform &&
            editSettings->toolMode != EditTool::ObjectMeta &&
            editSettings->toolMode != EditTool::Camera &&
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

        if (editSettings->toolMode != EditTool::Camera && editSettings->toolMode != EditTool::Light)
        {
            
            float turnInput = (int)inputData.current.buttons[BTN::DPAD_LEFT] -
                              (int)inputData.current.buttons[BTN::DPAD_RIGHT];

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
                    if (e.second->gameObjectType == GameObjectType::Sky ||
                        e.second->gameObjectType == GameObjectType::Terrain ||
                        e.second->gameObjectType == GameObjectType::Debug)
                        continue;

                    if (e.second->getRoughBoundingBox().Intersects(fpsCamera->getPosition(),
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
                    LOG(Severity::Info, "Picked GameObject " << selectedObject->name << ".");

                    std::vector<GameObject*> validGameObjects;

                    for (const auto& g : activeLevel->mGameObjects)
                    {
                        if (g.second->gameObjectType == GameObjectType::Static ||
                            g.second->gameObjectType == GameObjectType::Wall ||
                            g.second->gameObjectType == GameObjectType::Dynamic ||
                            g.second->gameObjectType == GameObjectType::Water)
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
                    auto box = editSettings->currentSelection->getRoughBoundingBox();

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

        /**BLUR TEST**/
        float blurriness = renderResource->getBlurFilter()->getSigma();

        blurriness += 1.25f * (inputData.current.buttons[BTN::DPAD_UP] ? 1.0f : inputData.current.buttons[BTN::DPAD_DOWN] ? -1.0f : 0.0f) * gt.DeltaTime();
        blurriness = MathHelper::clampH(blurriness, 0.0f, 2.5f);

        renderResource->getBlurFilter()->setSigma(blurriness);

        /**COMPOSITE COLOR TEST**/
        renderResource->addToCompositeColor(inputData.current.buttons[BTN::A] ? gt.DeltaTime() : inputData.current.buttons[BTN::B] ? -gt.DeltaTime() : 0.0f);

        if (fpsCameraMode)
        {
            fpsCamera->updateFPSCamera(inputData.current, gt);
        }
    }

    /*debug actions*/
    if (inputData.Released(BTN::DPAD_DOWN) && (settingsData->miscSettings.EditModeEnabled && ServiceProvider::getEditSettings()->toolMode == EditTool::Camera))
    {
        renderResource->toggleRoughHitBoxDraw();
    }


    /*first update camera, then the level*/
    ServiceProvider::getActiveCamera()->updateViewMatrix();

    activeLevel->update(gt);

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

    mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(offscreenRT->getResource(),
                                  D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_RENDER_TARGET));

    /*clear buffers*/
    mCommandList->ClearRenderTargetView(offscreenRT->getRtv(), Colors::LimeGreen, 0, nullptr);
    mCommandList->ClearDepthStencilView(getDepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

    /*set render target*/
    mCommandList->OMSetRenderTargets(1, &offscreenRT->getRtv(), true, &getDepthStencilView());

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



    ///*switch to post process root signature*/
    mCommandList->SetGraphicsRootSignature(renderResource->mPostProcessRootSignature.Get());

    if (renderResource->useMultColor)
    {
        mCommandList->SetPipelineState(renderResource->getPSO(RenderType::CompositeMult));
    }
    else
    {
        mCommandList->SetPipelineState(renderResource->getPSO(RenderType::Composite));
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
                            renderResource->getPSO(RenderType::BlurHorz), renderResource->getPSO(RenderType::BlurVert),
                            getCurrentBackBuffer());

        mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(getCurrentBackBuffer(), D3D12_RESOURCE_STATE_COPY_SOURCE,
                                      D3D12_RESOURCE_STATE_COPY_DEST));

        mCommandList->CopyResource(getCurrentBackBuffer(), blurFilter->getOutput());

        mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(getCurrentBackBuffer(),
                                      D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_RENDER_TARGET));
    }

    /*draw hud for the edit mode*/
    if (editModeHUD != nullptr)
        editModeHUD->draw();

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
