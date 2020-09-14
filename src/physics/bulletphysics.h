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

    /* set up bullet physics
    * @param gravity (y-acceleration)
    */
    BulletPhysics(float gravity = -9.81f);
    ~BulletPhysics();

    /* just calls the bullet physics step simulation
    @param delta time
    */
    bool simulateStep(float elapsedTime);

    /* add a game object as a rigid body to the dynamic world. modifies game object (add btRigidBody*)
    @param the game object
    @returns always true
    */
    bool addGameObject(GameObject& obj);

    /* delete all collision objects in the world / full reset for level change
    @returns always true
    */
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