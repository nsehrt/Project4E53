#include "inputmanager.h"
#include <chrono>
#include "../util/serviceprovider.h"


InputManager::InputManager()
{
    controller = std::make_unique<ControllerInput>();

    for (int i = 0; i < RING_SIZE; i++)
    {
        inputData.push_back(InputData());
        inputData[i].type = TYPE_GAMEPAD;
    }

    looped = true;
    inUse = -1;
}

InputManager::~InputManager()
{
}

void InputManager::Loop()
{
    ServiceProvider::getLogger()->setThreadName("inputThread");
    ServiceProvider::getLogger()->print<Severity::Info>("Starting the input manager.");

    HRESULT hr = CoInitialize(0);

    if (hr != S_OK)
    {
        ServiceProvider::getLogger()->print<Severity::Warning>("Failed to initialize COM!");
    }

    while (looped)
    {
        Update();
    }

    CoUninitialize();
    ServiceProvider::getLogger()->print<Severity::Info>("Stopping the input manager.");
}

void InputManager::Stop()
{
    looped = false;
}

InputSet InputManager::getInput()
{
    inUse = lastFinished.load();
    inputSet.current = inputData[inUse];
    return inputSet;
}

void InputManager::setPrevious(InputData data)
{
    inputSet.previous = data;
}

void InputManager::releaseInput()
{
    inUse = -1;
}

void InputManager::Update()
{
    int currentWorkedOn = 0;

    //read controller
    controller->Update();

    //read keyboard
    short keyboard[256];

    memset(keyboard, 0, sizeof(short) * 256);

    for(int i = 0; i < 256; i++)
    {
        keyboard[i] = GetAsyncKeyState(i);
    }

    /* work out index*/
    int tInUse = inUse.load();
    int tLastFinished = lastFinished.load();

    for (int i = 0; i < RING_SIZE; i++)
    {
        if (tInUse != i && tLastFinished != i)
        {
            currentWorkedOn = i;
            break;
        }
    }

    inputData[currentWorkedOn] = EmptyInputData;

    //read controller
    ControllerState* cs = nullptr;

    if(!useKeyboard)
    {
        cs = controller->getState(0);

        if(!cs->isConnected)
        {
            ServiceProvider::getLogger()->print<Severity::Warning>("Unable to find active xinput controller! Falling back to keyboard.");
            useKeyboard = true;

            return;
        }

        if(ServiceProvider::getSettings()->inputSettings.forceKeyboard)
        {
            useKeyboard = true;
        }
    }

    //controller
    if(!useKeyboard)
    {
        /*buttons*/
        if(cs->state.Gamepad.wButtons != 0)
        {
            if(cs->state.Gamepad.wButtons & XINPUT_GAMEPAD_A)
            {
                inputData[currentWorkedOn].buttons[BTN::A] = true;
            }
            if(cs->state.Gamepad.wButtons & XINPUT_GAMEPAD_B)
            {
                inputData[currentWorkedOn].buttons[BTN::B] = true;
            }
            if(cs->state.Gamepad.wButtons & XINPUT_GAMEPAD_X)
            {
                inputData[currentWorkedOn].buttons[BTN::X] = true;
            }
            if(cs->state.Gamepad.wButtons & XINPUT_GAMEPAD_Y)
            {
                inputData[currentWorkedOn].buttons[BTN::Y] = true;
            }
            if(cs->state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_UP)
            {
                inputData[currentWorkedOn].buttons[BTN::DPAD_UP] = true;
            }
            if(cs->state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_DOWN)
            {
                inputData[currentWorkedOn].buttons[BTN::DPAD_DOWN] = true;
            }
            if(cs->state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_LEFT)
            {
                inputData[currentWorkedOn].buttons[BTN::DPAD_LEFT] = true;
            }
            if(cs->state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_RIGHT)
            {
                inputData[currentWorkedOn].buttons[BTN::DPAD_RIGHT] = true;
            }
            if(cs->state.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER)
            {
                inputData[currentWorkedOn].buttons[BTN::LEFT_SHOULDER] = true;
            }
            if(cs->state.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER)
            {
                inputData[currentWorkedOn].buttons[BTN::RIGHT_SHOULDER] = true;
            }
            if(cs->state.Gamepad.wButtons & XINPUT_GAMEPAD_START)
            {
                inputData[currentWorkedOn].buttons[BTN::START] = true;
            }
            if(cs->state.Gamepad.wButtons & XINPUT_GAMEPAD_BACK)
            {
                inputData[currentWorkedOn].buttons[BTN::BACK] = true;
            }
            if(cs->state.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_THUMB)
            {
                inputData[currentWorkedOn].buttons[BTN::LEFT_THUMB] = true;
            }
            if(cs->state.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_THUMB)
            {
                inputData[currentWorkedOn].buttons[BTN::RIGHT_THUMB] = true;
            }
        }

        /*triggers*/

        inputData[currentWorkedOn].trigger[TRG::LEFT_TRIGGER] = controller->normalizeTriggers(cs->state.Gamepad.bLeftTrigger);
        inputData[currentWorkedOn].trigger[TRG::RIGHT_TRIGGER] = controller->normalizeTriggers(cs->state.Gamepad.bRightTrigger);

        inputData[currentWorkedOn].trigger[TRG::THUMB_LX] = controller->normalizeThumbs(cs->state.Gamepad.sThumbLX);
        inputData[currentWorkedOn].trigger[TRG::THUMB_LY] = controller->normalizeThumbs(cs->state.Gamepad.sThumbLY);
        inputData[currentWorkedOn].trigger[TRG::THUMB_RX] = controller->normalizeThumbs(cs->state.Gamepad.sThumbRX);
        inputData[currentWorkedOn].trigger[TRG::THUMB_RY] = controller->normalizeThumbs(cs->state.Gamepad.sThumbRY);

    }
    //keyboard
    else
    {

        inputData[currentWorkedOn].trigger[TRG::THUMB_LX] = keyboard[charToVKey('a')] ? -1.f : 0.00001f;
        inputData[currentWorkedOn].trigger[TRG::THUMB_LX] += keyboard[charToVKey('d')] ? 1.f : 0.00001f;
        inputData[currentWorkedOn].trigger[TRG::THUMB_LY] = keyboard[charToVKey('s')] ? -1.f : 0.00001f;
        inputData[currentWorkedOn].trigger[TRG::THUMB_LY] += keyboard[charToVKey('w')] ? 1.f : 0.00001f;
        inputData[currentWorkedOn].trigger[LEFT_TRIGGER] = keyboard[charToVKey('q')] ? 1.f : 0.f;
        inputData[currentWorkedOn].trigger[RIGHT_TRIGGER] = keyboard[VK_SHIFT] ? 1.f : 0.f;

        if(inputData[currentWorkedOn].trigger[TRG::THUMB_LX] < 0.001f &&
           inputData[currentWorkedOn].trigger[TRG::THUMB_LX] > -0.001f &&
           inputData[currentWorkedOn].trigger[TRG::THUMB_LY] < 0.001f &&
           inputData[currentWorkedOn].trigger[TRG::THUMB_LY] > -0.001f
           )
        {
            inputData[currentWorkedOn].trigger[TRG::THUMB_LX] = 0.0f;
            inputData[currentWorkedOn].trigger[TRG::THUMB_LY] = 0.0f;
        }

        /*buttons*/
        inputData[currentWorkedOn].buttons[BTN::A] = keyboard[VK_SPACE] || keyboard[VK_RETURN];
        inputData[currentWorkedOn].buttons[BTN::B] = keyboard[VK_CONTROL];
        inputData[currentWorkedOn].buttons[BTN::X] = keyboard[charToVKey('x')];
        inputData[currentWorkedOn].buttons[BTN::Y] = keyboard[charToVKey('y')];
        inputData[currentWorkedOn].buttons[DPAD_UP] = keyboard[charToVKey('w')];
        inputData[currentWorkedOn].buttons[DPAD_DOWN] = keyboard[charToVKey('s')];
        inputData[currentWorkedOn].buttons[DPAD_LEFT] = keyboard[VK_LEFT];
        inputData[currentWorkedOn].buttons[DPAD_RIGHT] = keyboard[VK_RIGHT];
        inputData[currentWorkedOn].buttons[LEFT_SHOULDER] = 0;
        inputData[currentWorkedOn].buttons[RIGHT_SHOULDER] = 0;
        inputData[currentWorkedOn].buttons[START] = keyboard[VK_RETURN];
        inputData[currentWorkedOn].buttons[BACK] = keyboard[VK_ESCAPE];
        inputData[currentWorkedOn].buttons[LEFT_THUMB] = 0;
        inputData[currentWorkedOn].buttons[RIGHT_THUMB] = 0;
    }



    // accumulate hold time by saving the timestamp a button was pressed
    auto currentTimeStamp = std::chrono::steady_clock::now();

    for(int i = 0; i < BUTTON_COUNT; i++)
    {
        if(!inputData[currentWorkedOn].buttons[i])
        {
            inputData[currentWorkedOn].buttonHoldTime[i] = currentTimeStamp;
        }
        else
        {
            inputData[currentWorkedOn].buttonHoldTime[i] = inputData[tLastFinished].buttonHoldTime[i];
        }
    }

    for(int i = 0; i < TRIGGER_COUNT; i++)
    {
        if(inputData[currentWorkedOn].trigger[i] <= 0.0f)
        {
            inputData[currentWorkedOn].triggerHoldTime[i] = currentTimeStamp;
        }
        else
        {
            inputData[currentWorkedOn].triggerHoldTime[i] = inputData[tLastFinished].triggerHoldTime[i];
        }

    }

    /*mark as finished*/
    lastFinished = currentWorkedOn;
}