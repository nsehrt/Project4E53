#pragma once

#include "../core/gameobject.h"
#include "../core/animationblender.h"

class BulletController;

class Character : public GameObject
{

    friend class BulletPhysics;

protected:

    enum class CharacterState
    {
        Idle,
        Walk,
        Run,
        JumpUp,
        JumpDown,
        Fall,
        Other
    };

    enum class CharacterAnimationState
    {
        Idle,
        Idle_2,
        Walk,
        Run,
        JumpInit,
        JumpUp,
        JumpDown,
        JumpRecover,
    };

    CharacterState currentCState = CharacterState::Idle;
    CharacterState previousCState = CharacterState::Idle;
    float timeSpentInAnimation = 0.0f;

public:

    explicit Character(const std::string& name, const std::string& model, int index, int skinnedIndex = -1);
    void setupController();
    btActionInterface* getController() const;
    void update(const GameTime& gt) override;

protected:

    AnimationBlender animationBlender;

    std::unique_ptr<BulletController> charController;

    float height = -1.0f;
};