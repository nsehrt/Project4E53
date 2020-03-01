#pragma once

#include "ControllerInput.h"
#include <vector>

#define BUTTON_COUNT 14
#define TRIGGER_COUNT 6
#define INPUT_MAX 5

/*types*/
#define TYPE_KEYBOARD 0
#define TYPE_GAMEPAD 1

/*buttons*/
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

/*triggers*/
#define LEFT_TRIGGER 0
#define RIGHT_TRIGGER 1
#define THUMB_LX 2
#define THUMB_LY 3
#define THUMB_RX 4
#define THUMB_RY 5



struct InputData
{
    int type;
    bool isConnected;
    bool buttons[BUTTON_COUNT];
    float trigger[TRIGGER_COUNT]; /* value between -1 - +1 or 0 - +1*/
};

class InputManager
{

public:

    InputManager();
    ~InputManager();

    void Update(float deltaTime);
    void UpdateMouse(POINT& p);
    void SetMouseSense(float sense);

    InputData* getInput(int index);
    InputData* getPrevInput(int index);
    
    bool ButtonPressed(int index, int button);
    bool ButtonReleased(int index, int button);

    void addUsedInput(int a);
    void clearUsedInput();
    bool usedInputActive = false;
private:
    ControllerInput* controller;

    InputData data[INPUT_MAX];
    InputData prevData[INPUT_MAX];
    std::vector<int> usedInputs;

    bool isUsedInput(int a);
    int charToVKey(char c);
    POINT mouseDelta;
    float mouseSense;
};