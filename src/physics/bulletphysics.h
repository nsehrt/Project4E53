#pragma once

#pragma comment(lib, "Bullet3Common.lib")
#pragma comment(lib, "BulletCollision.lib")
#pragma comment(lib, "BulletDynamics.lib")
#pragma comment(lib, "BulletInverseDynamics.lib")
#pragma comment(lib, "BulletSoftBody.lib")
#pragma comment(lib, "LinearMath.lib")

#include <btBulletDynamicsCommon.h>
#include "../physics/bulletcontroller.h"
#include "../core/gameobject.h"
#include "../core/character.h"

class GameObject;
class Terrain;

class BulletPhysics
{
    public:

    /* set up bullet physics
    * @param gravity (y-acceleration)
    */
    BulletPhysics(float gravity = -10.0f);
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

    /* add a character as a rigid body to the dynamic world.modifies character (add btRigidBody*)
    @param the game object
    @returns always true
    */
    bool addCharacter(Character& obj);

    /* add the terrain height map as a collision object to the physics world
    @param the terrain object
    @param the with the terrain associated game object
    @returns always true
    */
    bool addTerrain(Terrain& terrain, GameObject& obj);

    /* delete all collision objects in the world / full reset for level change
    @returns always true
    */
    bool reset();



    private:

    btCollisionShape* createShape(GameObject& obj);
    btCollisionShape* createBox(GameObject& obj);
    btCollisionShape* createCapsule(GameObject& obj);
    btCollisionShape* createSphere(GameObject& obj);
    btCollisionShape* createCylinder(GameObject& obj);

    float* convertedTerrainData = nullptr;

    static bool collisionCallback(btManifoldPoint& cp, const btCollisionObjectWrapper* obj1, int id1, int index1, const btCollisionObjectWrapper* obj2, int id2, int index2);

    //
    btBroadphaseInterface* m_broadphase = nullptr;
    btCollisionDispatcher* m_dispatcher = nullptr;
    btConstraintSolver* m_solver = nullptr;
    btDefaultCollisionConfiguration* m_collisionConfiguration = nullptr;
    btDiscreteDynamicsWorld* m_dynamicsWorld = nullptr;

};