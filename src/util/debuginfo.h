#pragma once

#include "../util/collector.h"

struct DebugInfo
{
    DebugInfo() : fpsData(128)
    {
        fpsData.fill(0.0f);
    }

    int DrawnGameObjects = 0;
    int DrawnShadowObjects = 0;
    float CurrentFPS = 0.0f;
    float Mspf = 0.0f;

    Collector<float> fpsData;
};