#pragma once

#include "../core/gameobject.h"

class BulletController;

enum class CharacterState
{
    Ground,
    Jump,
    Fall,
    Other
};

inline std::ostream& operator<< (std::ostream& os, const CharacterState& c)
{
    switch(c)
    {
        case CharacterState::Ground: os << "State::Ground"; break;
        case CharacterState::Jump: os << "State::Jump"; break;
        case CharacterState::Fall: os << "State::Fall"; break;
        case CharacterState::Other: os << "State::Other"; break;
        default: os << "State::???"; break;
    }

    return os;
}

enum class CharacterAnimationState
{
    Idle,
    Idle_2,
    Walk,
    Run,
    JumpInit,
    JumpUp,
    JumpDown,
    JumpRecover
};

class Character : public GameObject
{

    friend class BulletPhysics;

protected:

    CharacterState currentCState = CharacterState::Ground;
    CharacterState previousCState = CharacterState::Ground;
    float timeSpentInAnimation = 0.0f;

public:

    explicit Character(const std::string& name, const std::string& model, int index, int skinnedIndex = -1);
    void setupController();
    BulletController* getController() const;
    void update(const GameTime& gt) override;

protected:

    std::unique_ptr<BulletController> charController;

    float height = -1.0f;
};