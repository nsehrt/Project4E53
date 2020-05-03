#pragma once

#include "camera.h"
#include "../input/inputmanager.h"
#include "gametime.h"
#include "../util/serviceprovider.h"

class FPSCamera : public Camera
{
public:
    FPSCamera() = default;
    ~FPSCamera() = default;

    void updateFPSCamera(InputData& input, const GameTime& gt);
};