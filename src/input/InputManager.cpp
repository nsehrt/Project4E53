#include "InputManager.h"

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
    ServiceProvider::getVSLogger()->setThreadName("inputThread");
    ServiceProvider::getVSLogger()->print<Severity::Info>("Starting the input manager.");

    HRESULT hr = CoInitialize(0);

    if (hr != S_OK)
    {
        ServiceProvider::getVSLogger()->print<Severity::Warning>("Failed to initialize COM!");
    }

    while (looped)
    {
        Update();
    }

    CoUninitialize();
    ServiceProvider::getVSLogger()->print<Severity::Info>("Stopping the input manager.");

}



void InputManager::Stop()
{
    looped = false;
}

InputSet& InputManager::getInput()
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
    ControllerState *cs = controller->getState(0);

    if (!cs->isConnected)
    {
        ServiceProvider::getVSLogger()->print<Severity::Warning>("Unable to find active xinput controller!");
        return;
    }


    /*buttons*/
    if (cs->state.Gamepad.wButtons != 0)
    {
        if (cs->state.Gamepad.wButtons & XINPUT_GAMEPAD_A)
        {
            inputData[currentWorkedOn].buttons[BUTTON_A] = true;
        }
        if (cs->state.Gamepad.wButtons & XINPUT_GAMEPAD_B)
        {
            inputData[currentWorkedOn].buttons[BUTTON_B] = true;
        }
        if (cs->state.Gamepad.wButtons & XINPUT_GAMEPAD_X)
        {
            inputData[currentWorkedOn].buttons[BUTTON_X] = true;
        }
        if (cs->state.Gamepad.wButtons & XINPUT_GAMEPAD_Y)
        {
            inputData[currentWorkedOn].buttons[BUTTON_Y] = true;
        }
        if (cs->state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_UP)
        {
            inputData[currentWorkedOn].buttons[DPAD_UP] = true;
        }
        if (cs->state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_DOWN)
        {
            inputData[currentWorkedOn].buttons[DPAD_DOWN] = true;
        }
        if (cs->state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_LEFT)
        {
            inputData[currentWorkedOn].buttons[DPAD_LEFT] = true;
        }
        if (cs->state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_RIGHT)
        {
            inputData[currentWorkedOn].buttons[DPAD_RIGHT] = true;
        }
        if (cs->state.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER)
        {
            inputData[currentWorkedOn].buttons[LEFT_SHOULDER] = true;
        }
        if (cs->state.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER)
        {
            inputData[currentWorkedOn].buttons[RIGHT_SHOULDER] = true;
        }
        if (cs->state.Gamepad.wButtons & XINPUT_GAMEPAD_START)
        {
            inputData[currentWorkedOn].buttons[START] = true;
        }
        if (cs->state.Gamepad.wButtons & XINPUT_GAMEPAD_BACK)
        {
            inputData[currentWorkedOn].buttons[BACK] = true;
        }
        if (cs->state.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_THUMB)
        {
            inputData[currentWorkedOn].buttons[LEFT_THUMB] = true;
        }
        if (cs->state.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_THUMB)
        {
            inputData[currentWorkedOn].buttons[RIGHT_THUMB] = true;
        }
    }

    /*triggers*/

    inputData[currentWorkedOn].trigger[LEFT_TRIGGER] = controller->normalizeTriggers(cs->state.Gamepad.bLeftTrigger);
    inputData[currentWorkedOn].trigger[RIGHT_TRIGGER] = controller->normalizeTriggers(cs->state.Gamepad.bRightTrigger);

    inputData[currentWorkedOn].trigger[THUMB_LX] = controller->normalizeThumbs(cs->state.Gamepad.sThumbLX);
    inputData[currentWorkedOn].trigger[THUMB_LY] = controller->normalizeThumbs(cs->state.Gamepad.sThumbLY);
    inputData[currentWorkedOn].trigger[THUMB_RX] = controller->normalizeThumbs(cs->state.Gamepad.sThumbRX);
    inputData[currentWorkedOn].trigger[THUMB_RY] = controller->normalizeThumbs(cs->state.Gamepad.sThumbRY);


    /*mark as finished*/
    lastFinished = currentWorkedOn;
}