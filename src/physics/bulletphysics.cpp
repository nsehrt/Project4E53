#include "bulletphysics.h"
#include "../core/gameobject.h"


BulletPhysics::BulletPhysics(float gravity)
{

    m_collisionConfiguration = new btDefaultCollisionConfiguration();
    m_dispatcher = new btCollisionDispatcher(m_collisionConfiguration);
    m_broadphase = new btDbvtBroadphase();
    m_solver = new btSequentialImpulseConstraintSolver;
    m_dynamicsWorld = new btDiscreteDynamicsWorld(m_dispatcher, m_broadphase, m_solver, m_collisionConfiguration);

    m_dynamicsWorld->setGravity(btVector3(0.f, gravity, 0.f));

    gContactAddedCallback = collisionCallback;
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
    m_dynamicsWorld->stepSimulation(elapsedTime);

    return true;
}

bool BulletPhysics::addGameObject(GameObject& obj)
{
    // basic origin and rotation
    btTransform transform;
    transform.setIdentity();
    transform.setOrigin(btVector3(obj.getPosition().x,
                                  obj.getPosition().y,
                                  obj.getPosition().z));

    transform.setRotation(btQuaternion(obj.getRotation().x,
                                       obj.getRotation().y,
                                       obj.getRotation().z));

    //create collision shape
    btCollisionShape* shape = createShape(obj);

    //local inertia
    btVector3 inertia(0, 0, 0);

    if(obj.mass < 0.0f)
    {
        obj.mass = 0.0f;
    }

    if(obj.mass > 0.0f)
    {
        shape->calculateLocalInertia(obj.mass, inertia);
    }

    auto motionState = new btDefaultMotionState(transform);

    //put in info struct and create rigid body
    btRigidBody::btRigidBodyConstructionInfo bodyInfo(obj.mass, motionState, shape, inertia);
    btRigidBody* body = new btRigidBody(bodyInfo);
    obj.bulletBody = body;

    m_dynamicsWorld->addRigidBody(body);
    body->setUserPointer(&obj);

    return true;
}

bool BulletPhysics::reset()
{
    return true;
}

btCollisionShape* BulletPhysics::createShape(GameObject& obj)
{
    /*PLACEHOLDER*/
    const auto& coll = obj.getCollider();
    return new btBoxShape(btVector3(coll.getExtents().x, coll.getExtents().y, coll.getExtents().z));
}

bool BulletPhysics::collisionCallback(btManifoldPoint& cp, const btCollisionObjectWrapper* obj1, int id1, int index1, const btCollisionObjectWrapper* obj2, int id2, int index2)
{



    return false;
}
