#pragma once

#include <array>
#include "../util/randomizer.h"

struct Coin
{
    bool collected = false;
    float animationTime = 0.0f;
    bool animationFinished = false;
    int index = -1;
};

class Coins
{
    public:

    static constexpr int CoinCount = 8;
    static constexpr float minDistance = 6.5f;
    static constexpr float BaseHeight = 1.35f;
    static constexpr float FadeTime = 0.9f;
    static constexpr float BaseScale = 0.025f;

    static std::array < std::pair<int, int>, Coins::CoinCount> getCoinPlacement(Randomizer& rand, int w, int h)
    {

        std::array<std::pair<int, int>, Coins::CoinCount> placement{};

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

    private:
};