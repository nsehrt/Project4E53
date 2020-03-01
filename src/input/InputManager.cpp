#include "InputManager.h"

InputManager::InputManager()
{
    RtlSecureZeroMemory(&data[0], sizeof(InputData) * INPUT_MAX);
    RtlSecureZeroMemory(&prevData[0], sizeof(InputData) * INPUT_MAX);

    controller = new ControllerInput();
    mouseDelta = {};
    mouseSense = .5f;
}

InputManager::~InputManager()
{
}

void InputManager::Update(float deltaTime)
{
    //read devices first

    //read keyboard mouse to index 0
    short keyboard[256];
    
    memset(keyboard, 0, sizeof(short)*256);
    
    for (int i = 0; i < 256; i++)
    {
        keyboard[i] = GetAsyncKeyState(i);
    }

    //read controllers to 1-5
    controller->Update(deltaTime);

    //set previous
    for (int i = 0; i < INPUT_MAX; i++)
    {
        prevData[i] = data[i];
    }

    //clear previous
    RtlSecureZeroMemory(&data[0], sizeof(InputData)*(MAX_CONTROLLERS+1));

    //read keyboard

    if (isUsedInput(0))
    {

        data[0].type = TYPE_KEYBOARD;
        data[0].isConnected = true;

        if (data[0].isConnected)
        {
            /*trigger*/
            data[0].trigger[THUMB_LY] = keyboard[charToVKey('s')] ? -1.f : 0.f;
            data[0].trigger[THUMB_LY] += keyboard[charToVKey('w')] ? 1.f : 0.f;
            data[0].trigger[THUMB_LX] = keyboard[charToVKey('a')] ? -1.f : 0.f;
            data[0].trigger[THUMB_LX] += keyboard[charToVKey('d')] ? 1.f : 0.f;
            data[0].trigger[THUMB_RX] = (float)mouseDelta.x * mouseSense;
            data[0].trigger[THUMB_RY] = (float)mouseDelta.y * mouseSense;
            data[0].trigger[LEFT_TRIGGER] = keyboard[charToVKey('q')] ? 1.f : 0.f;
            data[0].trigger[RIGHT_TRIGGER] = keyboard[charToVKey('e')] ? 1.f : 0.f;

            /*buttons*/
            data[0].buttons[BUTTON_A] = keyboard[VK_SPACE];
            data[0].buttons[BUTTON_B] = keyboard[VK_CONTROL];
            data[0].buttons[BUTTON_X] = keyboard[charToVKey('x')];
            data[0].buttons[BUTTON_Y] = keyboard[charToVKey('y')];
            data[0].buttons[DPAD_UP] = keyboard[VK_UP];
            data[0].buttons[DPAD_DOWN] = keyboard[VK_DOWN];
            data[0].buttons[DPAD_LEFT] = keyboard[VK_LEFT];
            data[0].buttons[DPAD_RIGHT] = keyboard[VK_RIGHT];
            data[0].buttons[LEFT_SHOULDER] = 0;
            data[0].buttons[RIGHT_SHOULDER] = 0;
            data[0].buttons[START] = keyboard[VK_RETURN];
            data[0].buttons[BACK] = keyboard[VK_ESCAPE];
            data[0].buttons[LEFT_THUMB] = 0;
            data[0].buttons[RIGHT_THUMB] = 0;
        }
    }
    //read controllers
    for (int c = 0; c < MAX_CONTROLLERS; c++)
    {
        if (!isUsedInput(c + 1)) continue;
        //skip if not connected
        ControllerState *cs = controller->getState(c);

        if (!cs->isConnected)
        {
            continue;
        }

        data[c + 1].isConnected = true;
        data[c + 1].type = TYPE_GAMEPAD;


        /*buttons*/
        if (cs->state.Gamepad.wButtons != 0)
        {
            if (cs->state.Gamepad.wButtons & XINPUT_GAMEPAD_A)
            {
                data[c + 1].buttons[BUTTON_A] = true;
            }
            if (cs->state.Gamepad.wButtons & XINPUT_GAMEPAD_B)
            {
                data[c + 1].buttons[BUTTON_B] = true;
            }
            if (cs->state.Gamepad.wButtons & XINPUT_GAMEPAD_X)
            {
                data[c + 1].buttons[BUTTON_X] = true;
            }
            if (cs->state.Gamepad.wButtons & XINPUT_GAMEPAD_Y)
            {
                data[c + 1].buttons[BUTTON_Y] = true;
            }
            if (cs->state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_UP)
            {
                data[c + 1].buttons[DPAD_UP] = true;
            }
            if (cs->state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_DOWN)
            {
                data[c + 1].buttons[DPAD_DOWN] = true;
            }
            if (cs->state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_LEFT)
            {
                data[c + 1].buttons[DPAD_LEFT] = true;
            }
            if (cs->state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_RIGHT)
            {
                data[c + 1].buttons[DPAD_RIGHT] = true;
            }
            if (cs->state.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER)
            {
                data[c + 1].buttons[LEFT_SHOULDER] = true;
            }
            if (cs->state.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER)
            {
                data[c + 1].buttons[RIGHT_SHOULDER] = true;
            }
            if (cs->state.Gamepad.wButtons & XINPUT_GAMEPAD_START)
            {
                data[c + 1].buttons[START] = true;
            }
            if (cs->state.Gamepad.wButtons & XINPUT_GAMEPAD_BACK)
            {
                data[c + 1].buttons[BACK] = true;
            }
            if (cs->state.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_THUMB)
            {
                data[c + 1].buttons[LEFT_THUMB] = true;
            }
            if (cs->state.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_THUMB)
            {
                data[c + 1].buttons[RIGHT_THUMB] = true;
            }
        }

        /*triggers*/

        data[c + 1].trigger[LEFT_TRIGGER] = controller->normalizeTriggers(cs->state.Gamepad.bLeftTrigger);
        data[c + 1].trigger[RIGHT_TRIGGER] = controller->normalizeTriggers(cs->state.Gamepad.bRightTrigger);

        data[c + 1].trigger[THUMB_LX] = controller->normalizeThumbs(cs->state.Gamepad.sThumbLX);
        data[c + 1].trigger[THUMB_LY] = controller->normalizeThumbs(cs->state.Gamepad.sThumbLY);
        data[c + 1].trigger[THUMB_RX] = controller->normalizeThumbs(cs->state.Gamepad.sThumbRX);
        data[c + 1].trigger[THUMB_RY] = controller->normalizeThumbs(cs->state.Gamepad.sThumbRY);
    }


}

void InputManager::UpdateMouse(POINT& p)
{
    p.y *= -1;
    mouseDelta = p;
}

void InputManager::SetMouseSense(float sense)
{
    if (sense > 0)
    {
        mouseSense = sense;
    }
    else
    {
        mouseSense = 1.f;
    }
    
}

InputData* InputManager::getInput(int index)
{
    return &data[index];
}

InputData* InputManager::getPrevInput(int index)
{
    return &prevData[index];
}

bool InputManager::ButtonPressed(int index, int button)
{
    return data[index].buttons[button] && !prevData[index].buttons[button];
}

bool InputManager::ButtonReleased(int index, int button)
{
    return !data[index].buttons[button] && prevData[index].buttons[button];
}

void InputManager::addUsedInput(int a)
{
    usedInputs.push_back(a);
}

void InputManager::clearUsedInput()
{
    usedInputs.clear();
}



bool InputManager::isUsedInput(int a)
{
    if (!usedInputActive) return true;

    for (int& i : usedInputs)
    {
        if (i == a) return true;
    }
    return false;
}

int InputManager::charToVKey(char c)
{
    int k = (int)c - 32;
    return (k < 65 || k > 90) ? -1 : k;
}
