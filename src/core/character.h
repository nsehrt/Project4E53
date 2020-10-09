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
        Jump,
        Roll,
        Other
    };


public:

    explicit Character(const std::string& name, const std::string& model, int index, int skinnedIndex = -1);
    void setupController();
    btActionInterface* getController() const;
    void update(const GameTime& gt) override;

protected:

    AnimationBlender animationBlender;

    CharacterState currentCState = CharacterState::Idle;
    CharacterState previousCState = CharacterState::Idle;

    std::unique_ptr<BulletController> charController;

    float walkSpeed = 2.5f;
    float runSpeed = 5.5f;
    float movementRampTime = 0.275f;

    float timeIdle = 0.0f;
    float timeMoving = 0.0f;

    float height = -1.0f;
};