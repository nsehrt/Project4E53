/* ControllerInput.h

polls input and status data from up to 4 controllers

    Usage:
    i = new ControllerInput();
    ...
    i->Update(deltaTime);
    ...
    *cstatei = i->getState(0);

*/

#pragma once

#include <windows.h>
#include <XInput.h>
#pragma comment(lib,"xinput.lib")
#pragma warning(disable : 4995)
#define MAX_CONTROLLERS 1
#define INPUT_DEADZONE (0.24f* float(0x7FFF))
/*time between checking capabilites/battery level
  same controller is polled every CAP_BATTERY_POLL_TIME * COUNTOFCONTROLLER*/
#define CAP_BATTERY_POLL_TIME 60.f

struct ControllerState
{
    XINPUT_STATE state;
    XINPUT_BATTERY_INFORMATION batteryInfo;
    XINPUT_CAPABILITIES capabilities;
    bool isConnected;
};

/*XINPUT DOCU*/

/*state.wbuttons
    check these values with cs->state.Gamepad.wButtons & XINPUT_GAMEPAD_A
Device button 	Bitmask
XINPUT_GAMEPAD_DPAD_UP 	0x0001
XINPUT_GAMEPAD_DPAD_DOWN 	0x0002
XINPUT_GAMEPAD_DPAD_LEFT 	0x0004
XINPUT_GAMEPAD_DPAD_RIGHT 	0x0008
XINPUT_GAMEPAD_START 	0x0010
XINPUT_GAMEPAD_BACK 	0x0020
XINPUT_GAMEPAD_LEFT_THUMB 	0x0040
XINPUT_GAMEPAD_RIGHT_THUMB 	0x0080
XINPUT_GAMEPAD_LEFT_SHOULDER 	0x0100
XINPUT_GAMEPAD_RIGHT_SHOULDER 	0x0200
XINPUT_GAMEPAD_A 	0x1000
XINPUT_GAMEPAD_B 	0x2000
XINPUT_GAMEPAD_X 	0x4000
XINPUT_GAMEPAD_Y 	0x8000
*/

/*
bLeftTrigger

The current value of the left trigger analog control. The value is between 0 and 255.

bRightTrigger

The current value of the right trigger analog control. The value is between 0 and 255.

sThumbLX

Left thumbstick x-axis value. Each of the thumbstick axis members is a signed value between -32768 and 32767 describing the position of the thumbstick. A value of 0 is centered. Negative values signify down or to the left. Positive values signify up or to the right. The constants XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE or XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE can be used as a positive and negative value to filter a thumbstick input.

sThumbLY

Left thumbstick y-axis value. The value is between -32768 and 32767.

sThumbRX

Right thumbstick x-axis value. The value is between -32768 and 32767.

sThumbRY

Right thumbstick y-axis value. The value is between -32768 and 32767.
*/

/*batteryinfo.BatteryLevel values
BATTERY_LEVEL_EMPTY
BATTERY_LEVEL_LOW
BATTERY_LEVEL_MEDIUM
BATTERY_LEVEL_FULL
*/

/*capabilities flags
XINPUT_CAPS_VOICE_SUPPORTED 	Device has an integrated voice device.
XINPUT_CAPS_FFB_SUPPORTED 	Device supports force feedback functionality. Note that these force-feedback features beyond rumble are not currently supported through XINPUT on Windows.
XINPUT_CAPS_WIRELESS 	Device is wireless.
XINPUT_CAPS_PMD_SUPPORTED 	Device supports plug-in modules. Note that plug-in modules like the text input device (TID) are not supported currently through XINPUT on Windows.
XINPUT_CAPS_NO_NAVIGATION 	Device lacks menu navigation buttons (START, BACK, DPAD).
*/

class ControllerInput
{
public:
    explicit ControllerInput();
    ~ControllerInput();

    void Update();
    void Enable();
    void Disable();
    bool isConnected(int index);
    ControllerState* getState(int index);
    float normalizeThumbs(int in);
    float normalizeTriggers(int in);

private:
    ControllerState controllers[MAX_CONTROLLERS];
    void Update_Internal(bool init);
    void resetControllerData(int index);

    float cTimer;
    bool isInit;
    int currentUpdate;
};