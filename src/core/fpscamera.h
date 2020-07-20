#pragma once

#include "..\core\camera.h"
#include "..\input\inputmanager.h"
#include "..\core\gametime.h"

class FPSCamera : public Camera
{
public:
    FPSCamera() = default;
    ~FPSCamera() = default;

    void updateFPSCamera(InputData& input, const GameTime& gt);
};