#pragma once

#include "character.h"
#include "../input/inputmanager.h"
#include "coins.h"

class Player : public Character
{
    friend class BulletController;

public:

    explicit Player(const std::string& model);
    void stickToTerrain();
    virtual void update(const GameTime& gt) override;

    int coinCount() const
    {
        int val = 0;
        for(const auto& b : coins)
            if(b.collected)
                val++;
        return val;
    }

    std::array<Coin, Coins::CoinCount> coins{};

    void resetCoins()
    {
        std::fill(coins.begin(), coins.end(), Coin{});
        for(int i = 0; i < Coins::CoinCount; i++)
        {
            coins[i].index = i;
        }
    }

private:

    enum class IdleAnimation
    {
        Default,
        Scratch
    };

};