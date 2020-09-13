#include "bulletphysics.h"
#include <iostream>


BulletPhysics::BulletPhysics(float gravity)
{

    m_collisionConfiguration = new btDefaultCollisionConfiguration();
    m_dispatcher = new btCollisionDispatcher(m_collisionConfiguration);
    m_broadphase = new btDbvtBroadphase();
    m_solver = new btSequentialImpulseConstraintSolver;
    m_dynamicsWorld = new btDiscreteDynamicsWorld(m_dispatcher, m_broadphase, m_solver, m_collisionConfiguration);

    m_dynamicsWorld->setGravity(btVector3(0.f, gravity, 0.f));

}

BulletPhysics::~BulletPhysics()
{

    delete m_dynamicsWorld;
    delete m_solver;
    delete m_broadphase;
    delete m_dispatcher;
    delete m_collisionConfiguration;
}



bool BulletPhysics::simulateStep(float elapsedTime)
{
    if(m_dynamicsWorld)
    {
        m_dynamicsWorld->stepSimulation(elapsedTime);
    }
    return false;
}
