#pragma once

#include "../core/gameobject.h"


class Character : public GameObject
{
public:

    explicit Character(const std::string& name, const std::string& model, int index, int skinnedIndex = -1);

    void update(const GameTime& gt) override;

private:

};