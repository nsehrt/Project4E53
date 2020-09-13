#pragma once

#pragma comment(lib, "Bullet3Common.lib")
#pragma comment(lib, "BulletCollision.lib")
#pragma comment(lib, "BulletDynamics.lib")
#pragma comment(lib, "BulletInverseDynamics.lib")
#pragma comment(lib, "BulletSoftBody.lib")
#pragma comment(lib, "LinearMath.lib")

#include <btBulletDynamicsCommon.h>
#include "../core/gameobject.h"

class GameObject;

class BulletPhysics
{
    public:

    BulletPhysics(float gravity = -9.81f);
    ~BulletPhysics();

    bool simulateStep(float elapsedTime);
    bool addGameObject(GameObject& obj);    
    bool reset();

    private:

    btCollisionShape* createShape(GameObject& obj);
    static bool collisionCallback(btManifoldPoint& cp, const btCollisionObjectWrapper* obj1, int id1, int index1, const btCollisionObjectWrapper* obj2, int id2, int index2);

    //
    btBroadphaseInterface* m_broadphase = nullptr;
    btCollisionDispatcher* m_dispatcher = nullptr;
    btConstraintSolver* m_solver = nullptr;
    btDefaultCollisionConfiguration* m_collisionConfiguration = nullptr;
    btDiscreteDynamicsWorld* m_dynamicsWorld = nullptr;

};