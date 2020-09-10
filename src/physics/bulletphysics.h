#pragma once

#pragma comment(lib, "Bullet3Common.lib")
#pragma comment(lib, "BulletCollision.lib")
#pragma comment(lib, "BulletDynamics.lib")
#pragma comment(lib, "BulletInverseDynamics.lib")
#pragma comment(lib, "BulletSoftBody.lib")
#pragma comment(lib, "LinearMath.lib")

#include <btBulletDynamicsCommon.h>

class BulletPhysics
{
    public:

    BulletPhysics() = default;
    ~BulletPhysics();

    bool init(float gravity = 9.81f);
    bool simulateStep(float elapsedTime);

    private:

    btBroadphaseInterface* m_broadphase = nullptr;
    btCollisionDispatcher* m_dispatcher = nullptr;
    btConstraintSolver* m_solver = nullptr;
    btDefaultCollisionConfiguration* m_collisionConfiguration = nullptr;
    btDiscreteDynamicsWorld* m_dynamicsWorld = nullptr;
};