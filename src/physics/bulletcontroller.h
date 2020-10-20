#pragma once

#include <btBulletDynamicsCommon.h>
#include <BulletDynamics/Dynamics/btActionInterface.h>

class GameObject;

class BulletController final : public btActionInterface
{

    public:

    explicit BulletController(btRigidBody* body);
    ~BulletController() = default;

    void updateAction(btCollisionWorld* collisionWorld, btScalar deltaTimeStep) override final;
    void debugDraw(btIDebugDraw* debugDrawer) override final; /*not used*/

    private:

    void setupBody();


    GameObject* objectUnderPlayer = nullptr;
    btRigidBody* rigidBody = nullptr;


    /*movement properties*/
    btVector3 velocity{}; 

    bool onGround = false;
    bool previousOnGround = false;

    float distanceToGround = 0.0f;
    float previousDistanceToGround = 0.0f;

    float timeIdle = 0.0f;
    float timeMoving = 0.0f;
    float timeInCurrentState = 0.0f;


    const float turnSmoothTime = 6.25f;
    const float rayLength = 50.0f;
    const float offGroundThreshold = 0.2f;

    const float walkSpeed = 2.5f;
    const float runSpeed = 5.5f;
    const float movementRampTime = 0.275f;

    const btVector3 onGroundGravity = { 0.0f,-50.0f,0.0f };
    const btVector3 inAirGravity = { 0.0f,-9.81f,0.0f };
    const btVector3 onIdleGravity = { 0.0f,0.0f,0.0f };
};