#pragma once

#include "character.h"
#include "../input/inputmanager.h"

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
        for(const bool b : coinsCollected)
            if(b)
                val++;
        return val;
    }

    std::array<bool,8> coinsCollected{};

private:

    enum class IdleAnimation
    {
        Default,
        Scratch
    };

    void resetCoins()
    {
        std::fill(coinsCollected.begin(), coinsCollected.end(), false);
    }



};