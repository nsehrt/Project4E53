#include "InputManager.h"

InputManager::InputManager()
{
    controller = std::make_unique<ControllerInput>();

    for (int i = 0; i < RING_SIZE; i++)
    {
        inputData.push_back(new InputData());
    }

    looped = true;
}

InputManager::~InputManager()
{
    for (int i = 0; i < RING_SIZE; i++)
    {
        delete inputData[i];
    }
}

void InputManager::Loop()
{
    ServiceProvider::getVSLogger()->setThreadName("inputThread");
    ServiceProvider::getVSLogger()->print<Severity::Info>("Starting input manager.");

    while (looped)
    {
        //Update();
    }

    ServiceProvider::getVSLogger()->print<Severity::Info>("Stopping input manager.");
}



void InputManager::Stop()
{
    looped = false;
}

void InputManager::Update()
{

    //read controller
    controller->Update();

    //set previous
    for (int i = 0; i < INPUT_MAX; i++)
    {
       //prevData[i] = data[i];
    }

    /* work out index*/



    inputData[currentWorkedOn]->previous = prevData;

    //read controller
    ControllerState *cs = controller->getState(0);

    if (!cs->isConnected)
    {
        ServiceProvider::getVSLogger()->print<Severity::Warning>("Unable to find active xinput controller!");
        return;
    }

    InputS data;


    /*buttons*/
    if (cs->state.Gamepad.wButtons != 0)
    {
        if (cs->state.Gamepad.wButtons & XINPUT_GAMEPAD_A)
        {
            data.buttons[BUTTON_A] = true;
        }
        if (cs->state.Gamepad.wButtons & XINPUT_GAMEPAD_B)
        {
            data.buttons[BUTTON_B] = true;
        }
        if (cs->state.Gamepad.wButtons & XINPUT_GAMEPAD_X)
        {
            data.buttons[BUTTON_X] = true;
        }
        if (cs->state.Gamepad.wButtons & XINPUT_GAMEPAD_Y)
        {
            data.buttons[BUTTON_Y] = true;
        }
        if (cs->state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_UP)
        {
            data.buttons[DPAD_UP] = true;
        }
        if (cs->state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_DOWN)
        {
            data.buttons[DPAD_DOWN] = true;
        }
        if (cs->state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_LEFT)
        {
            data.buttons[DPAD_LEFT] = true;
        }
        if (cs->state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_RIGHT)
        {
            data.buttons[DPAD_RIGHT] = true;
        }
        if (cs->state.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER)
        {
            data.buttons[LEFT_SHOULDER] = true;
        }
        if (cs->state.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER)
        {
            data.buttons[RIGHT_SHOULDER] = true;
        }
        if (cs->state.Gamepad.wButtons & XINPUT_GAMEPAD_START)
        {
            data.buttons[START] = true;
        }
        if (cs->state.Gamepad.wButtons & XINPUT_GAMEPAD_BACK)
        {
            data.buttons[BACK] = true;
        }
        if (cs->state.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_THUMB)
        {
            data.buttons[LEFT_THUMB] = true;
        }
        if (cs->state.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_THUMB)
        {
            data.buttons[RIGHT_THUMB] = true;
        }
    }

    /*triggers*/

    data.trigger[LEFT_TRIGGER] = controller->normalizeTriggers(cs->state.Gamepad.bLeftTrigger);
    data.trigger[RIGHT_TRIGGER] = controller->normalizeTriggers(cs->state.Gamepad.bRightTrigger);

    data.trigger[THUMB_LX] = controller->normalizeThumbs(cs->state.Gamepad.sThumbLX);
    data.trigger[THUMB_LY] = controller->normalizeThumbs(cs->state.Gamepad.sThumbLY);
    data.trigger[THUMB_RX] = controller->normalizeThumbs(cs->state.Gamepad.sThumbRX);
    data.trigger[THUMB_RY] = controller->normalizeThumbs(cs->state.Gamepad.sThumbRY);

}


InputData* InputManager::getInput()
{
    return inputData[inUse];
}

void InputManager::Lock()
{
    lockUsage.lock();
    inUse = lastFinished;
}

void InputManager::Unlock()
{
    inUse = -1;
    lockUsage.unlock();
}

//bool InputManager::ButtonPressed(int index, int button)
//{
//    return data[index].buttons[button] && !prevData[index].buttons[button];
//}
//
//bool InputManager::ButtonReleased(int index, int button)
//{
//    return !data[index].buttons[button] && prevData[index].buttons[button];
//}


//int InputManager::charToVKey(char c)
//{
//    int k = (int)c - 32;
//    return (k < 65 || k > 90) ? -1 : k;
//}
