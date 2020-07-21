#pragma once

#include "character.h"
#include "../input/inputmanager.h"

class Player : public Character
{
public:

    Player(const std::string& model);

    void update(const InputSet& input, const GameTime& gt);

private:
    const float walkSpeed = 2.5f;
    const float runSpeed = 5.5f;

    bool noClip = false;


};