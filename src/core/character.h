#pragma once

#include "../core/gameobject.h"
#include "../core/animationblender.h"


class Character : public GameObject
{

protected:

    enum class CharacterState
    {
        Idle,
        Walk,
        Run,
        Jump,
        Roll,
        Other
    };


public:

    explicit Character(const std::string& name, const std::string& model, int index, int skinnedIndex = -1);

    void update(const GameTime& gt) override;

protected:

    AnimationBlender animationBlender;

    CharacterState currentCState = CharacterState::Idle;
    CharacterState previousCState = CharacterState::Idle;

    float walkSpeed = 2.5f;
    float runSpeed = 5.5f;
    float turnSmoothTime = 6.25f;
    float movementRampTime = 0.275f;

    float timeIdle = 0.0f;
    float timeMoving = 0.0f;

};