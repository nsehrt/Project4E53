#pragma once

#include <btBulletDynamicsCommon.h>
#include <BulletDynamics/Dynamics/btActionInterface.h>
#include "../util/mathhelper.h"
#include "../core/player.h"

class GameObject;

class BulletController final : public btActionInterface
{

    public:

    explicit BulletController(btRigidBody* body);
    ~BulletController() = default;

    void updateAction(btCollisionWorld* collisionWorld, btScalar deltaTimeStep) override final;
    void debugDraw(btIDebugDraw* debugDrawer) override final; /*not used*/

    void jump();
    void run(bool value = false);
    void setMovement(const DirectX::XMFLOAT2& direction, const float magnitude);

    private:

    void setupBody();
    void setState(const CharacterState state);
    void resetMovementParameters();

    GameObject* objectUnderPlayer = nullptr;
    btRigidBody* rigidBody = nullptr;


    /*movement properties*/
    DirectX::XMFLOAT2 velocity{};
    DirectX::XMFLOAT2 previousVelocity{};

    float inputMagnitude = 0.0f;
    DirectX::XMFLOAT2 inputDirection;

    bool onGround = false;
    bool previousOnGround = false;
    bool jumpedThisFrame = false;

    bool pressedJump = false;
    bool pressedRun = false;
    bool pressedRoll = false;

    float distanceToGround = 0.0f;
    float previousDistanceToGround = 0.0f;

    float timeIdle = 0.0f;
    float timeMoving = 0.0f;
    float timeInCurrentState = 0.0f;


    const float turnSmoothTime = 10.5f;
    const float rayLength = 50.0f;
    const float offGroundThreshold = 0.2f;
    const float minimumJumpTime = 0.1f;
    const float fallJumpGracePeriod = 0.18f;

    const float walkSpeed = 4.0f;
    const float runSpeed = 6.5f;
    const float movementRampTime = 0.275f;
    const float jumpUpVelocity = 12.5f;

    const btVector3 onGroundGravity = { 0.0f,-50.0f,0.0f };
    const btVector3 inAirGravity = { 0.0f,-19.62f,0.0f };
    const btVector3 onIdleGravity = { 0.0f,0.0f,0.0f };
};