#pragma once

#include "character.h"
#include "../input/inputmanager.h"

class Player : public Character
{
public:

    explicit Player(const std::string& model);

    void update(const InputSet& input, const GameTime& gt);
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