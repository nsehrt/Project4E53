#include "bulletphysics.h"
#include "../core/gameobject.h"
#include "../util/serviceprovider.h"

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

    body->setUserPointer(&obj);

    //enabl callback function
    //body->setCollisionFlags(body->getCollisionFlags() | btCollisionObject::CF_CUSTOM_MATERIAL_CALLBACK);

    m_dynamicsWorld->addRigidBody(body);


    return true;
}

// TODO
bool BulletPhysics::reset()
{
    return true;
}

btCollisionShape* BulletPhysics::createShape(GameObject& obj)
{
    switch(obj.shapeType)
    {
        case BOX_SHAPE_PROXYTYPE: return createBox(obj); break;
        case CAPSULE_SHAPE_PROXYTYPE: return createCapsule(obj); break;
        case SPHERE_SHAPE_PROXYTYPE: return createSphere(obj); break;
        case CYLINDER_SHAPE_PROXYTYPE: return createCylinder(obj); break;
        default: LOG(Severity::Warning, "Game object " << obj.Name << ": invalid collision shape!");
    }


    return createBox(obj);
}

btCollisionShape* BulletPhysics::createBox(GameObject& obj)
{
    return new btBoxShape(btVector3(obj.extents.x, obj.extents.y, obj.extents.z));
}

btCollisionShape* BulletPhysics::createCapsule(GameObject& obj)
{
    return new btCapsuleShape(obj.extents.x, obj.extents.y);
}

btCollisionShape* BulletPhysics::createSphere(GameObject& obj)
{
    return new btSphereShape(obj.extents.x * 2.0f);
}

btCollisionShape* BulletPhysics::createCylinder(GameObject& obj)
{
    return new btCylinderShape(btVector3(obj.extents.x * 2.0f, obj.extents.y * 2.0f, obj.extents.z * 2.0f));
}

bool BulletPhysics::collisionCallback(btManifoldPoint& cp, const btCollisionObjectWrapper* obj1, int id1, int index1, const btCollisionObjectWrapper* obj2, int id2, int index2)
{
    GameObject* a = (GameObject*)obj1->getCollisionObject()->getUserPointer();
    GameObject* b = (GameObject*)obj2->getCollisionObject()->getUserPointer();

    LOG(Severity::Debug, a->Name << " " << b->Name);

    return false;
}
