#pragma once

#include "ControllerInput.h"
#include <vector>
#include <atomic>
#include <mutex>
#include "../util/serviceprovider.h"
#include "../core/gametime.h"

#define BUTTON_COUNT 14
#define TRIGGER_COUNT 6
#define INPUT_MAX 5

/*types*/
#define TYPE_KEYBOARD 0
#define TYPE_GAMEPAD 1

#define RING_SIZE 12

/*14 buttons*/

enum BTN : int
{
    A = 0,
    B,
    X,
    Y,
    DPAD_UP,
    DPAD_DOWN,
    DPAD_LEFT,
    DPAD_RIGHT,
    LEFT_SHOULDER,
    RIGHT_SHOULDER,
    START,
    BACK,
    LEFT_THUMB,
    RIGHT_THUMB
};

/*6 triggers*/

enum TRG : int
{
    LEFT_TRIGGER = 0,
    RIGHT_TRIGGER,
    THUMB_LX,
    THUMB_LY,
    THUMB_RX,
    THUMB_RY
};

struct InputData
{
    int type = TYPE_GAMEPAD;
    bool buttons[BUTTON_COUNT] = {};

    float trigger[TRIGGER_COUNT] = {};
};

struct InputSet
{
    InputData current;
    InputData previous;

    bool Pressed(int button) const
    {
        return current.buttons[button] && !previous.buttons[button];
    }

    bool Released(int button) const
    {
        return !current.buttons[button] && previous.buttons[button];
    }
};

class InputManager
{

public:

    InputManager();
    ~InputManager();

    void Loop();
    void Stop();

    void Update();

    InputSet& getInput();
    void setPrevious(InputData data);

    void releaseInput();

private:

    std::atomic<bool> looped;
    std::atomic<int> inUse, lastFinished;

    std::unique_ptr<ControllerInput> controller;

    std::vector<InputData> inputData;
    InputSet inputSet = {};

    const struct InputData EmptyInputData = {};
};