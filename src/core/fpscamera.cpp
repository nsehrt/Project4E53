#include "fpscamera.h"

void FPSCamera::updateFPSCamera(InputData& inputData, const GameTime& gt)
{
    Settings* settingsData = ServiceProvider::getSettings();

    float lx = inputData.trigger[THUMB_LX] * settingsData->inputSettings.FPSCameraSpeed;
    float ly = inputData.trigger[THUMB_LY] * settingsData->inputSettings.FPSCameraSpeed;
    float rx = inputData.trigger[THUMB_RX] * settingsData->inputSettings.Sensitivity;
    float ry = (settingsData->inputSettings.InvertYAxis ? -1 : 1) * inputData.trigger[THUMB_RY] * settingsData->inputSettings.Sensitivity;

    pitch(ry * gt.DeltaTime());
    rotateY(rx * gt.DeltaTime());

    walk(ly * gt.DeltaTime());
    strafe(lx * gt.DeltaTime());

    mTarget = getPosition3f();
}