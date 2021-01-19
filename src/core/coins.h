#pragma once

#include <array>
#include "../util/randomizer.h"

class Coins
{
    public:

    static std::array<std::pair<int, int>, 4> getCoinPlacement(Randomizer& rand, int w, int h)
    {

        std::array<std::pair<int, int>, 4> placement{};

        for(int i = 0; i < placement.size(); i++)
        {

            int x = 0;
            int y = 0;

            while(true)
            {
                bool alreadyExists = false;
                x = rand.nextInt(w - 1);
                y = rand.nextInt(h - 1);

                for(int j = 0; j < i; j++)
                {
                    if(distance(placement[j].first, placement[j].second, x, y) < minDistance)
                    {
                        alreadyExists = true;
                        break;
                    }

                }

                if(!alreadyExists)
                {
                    placement[i].first = x;
                    placement[i].second = y;
                    break;
                }

            }

        }

        return placement;
    }

    static float distance(int x1, int y1, int x2, int y2)
    {
        return std::sqrtf(static_cast<float>(((x1 - x2) * (x1 - x2)) + ((y1 - y2) * (y1 - y2))));
    }

    static constexpr int CoinCount = 4;
    static constexpr float minDistance = 8.0f;
    static constexpr float BaseHeight = 1.25f;

    private:
};