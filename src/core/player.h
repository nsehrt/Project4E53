#pragma once

#include "character.h"
#include "../input/inputmanager.h"

class Player : public Character
{
public:

    explicit Player(const std::string& model);

    void update(const InputSet& input, const GameTime& gt);

private:


};