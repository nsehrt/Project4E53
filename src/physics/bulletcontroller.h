#pragma once

#include <BulletDynamics/Dynamics/btActionInterface.h>
class btCapsuleShape;
class btRigidBody;


class BulletController final : public btActionInterface
{

    public:

    explicit BulletController(btRigidBody* body);
    ~BulletController() = default;
    
    void updateAction(btCollisionWorld* collisionWorld, btScalar deltaTimeStep) override final;
    void debugDraw(btIDebugDraw* debugDrawer) override final; /*not used*/

    private:

    void setupBody();


    btRigidBody* rigidBody = nullptr;
    float bodyGravity = 0.0f;
    float turnSmoothTime = 6.25f;
};