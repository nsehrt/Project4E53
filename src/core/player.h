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

private:

    enum class IdleAnimation
    {
        Default,
        Scratch
    };

    float baseStamina = 100.0f;
    int health = 1;
    int maxHealth = 1;
};