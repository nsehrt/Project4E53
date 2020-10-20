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

    bool onGround = false;
    float maxDistanceFromGround = 0.0f;
    GameObject* objectUnderPlayer = nullptr;

    btRigidBody* rigidBody = nullptr;
    float bodyGravity = 0.0f;

    const float turnSmoothTime = 6.25f;
    const float rayLength = 100.0f;
    const float slopeThresholdPerSecond = 0.5f;
};

class GroundCheck : public btCollisionWorld::ContactResultCallback
{
	public:
    GroundCheck(const BulletController* controller,
                const btCollisionWorld* world)
        : mController(controller), mWorld(world)
    {
    }

	btScalar addSingleResult(btManifoldPoint& cp,
							 const btCollisionObjectWrapper* colObj0, int partId0, int index0,
							 const btCollisionObjectWrapper* colObj1, int partId1, int index1);

	bool mOnGround = false;
	btVector3 mGroundPoint;

	private:
	void checkGround(const btManifoldPoint& cp);

	const BulletController* mController;
	const btCollisionWorld* mWorld;
};

