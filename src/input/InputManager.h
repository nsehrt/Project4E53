#pragma once

#include "ControllerInput.h"
#include <vector>
#include <atomic>
#include <mutex>
#include "../util/serviceprovider.h"
#include "../core/gametime.hpp"

#define BUTTON_COUNT 14
#define TRIGGER_COUNT 6
#define INPUT_MAX 5

/*types*/
#define TYPE_KEYBOARD 0
#define TYPE_GAMEPAD 1

/*14 buttons*/
#define BUTTON_A 0
#define BUTTON_B 1
#define BUTTON_X 2
#define BUTTON_Y 3
#define DPAD_UP 4
#define DPAD_DOWN 5
#define DPAD_LEFT 6
#define DPAD_RIGHT 7
#define LEFT_SHOULDER 8
#define RIGHT_SHOULDER 9
#define START 10
#define BACK 11
#define LEFT_THUMB 12
#define RIGHT_THUMB 13

/*6 triggers*/
#define LEFT_TRIGGER 0
#define RIGHT_TRIGGER 1
#define THUMB_LX 2
#define THUMB_LY 3
#define THUMB_RX 4
#define THUMB_RY 5

#define RING_SIZE 12

struct InputData
{
    int type;
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