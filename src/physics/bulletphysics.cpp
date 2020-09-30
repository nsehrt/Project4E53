#include "bulletphysics.h"
#include <BulletCollision/CollisionShapes/btHeightfieldTerrainShape.h>
#include "../core/gameobject.h"
#include "../core/terrain.h"
#include "../util/mathhelper.h"
#include "../util/serviceprovider.h"

BulletPhysics::BulletPhysics(float gravity)
{

    LOG(Severity::Info, "Setting up bullet physics.");

    m_collisionConfiguration = new btDefaultCollisionConfiguration();
    m_dispatcher = new btCollisionDispatcher(m_collisionConfiguration);
    m_broadphase = new btDbvtBroadphase();
    m_solver = new btSequentialImpulseConstraintSolver();
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

    delete convertedTerrainData;

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


    transform.setRotation(btQuaternion(obj.getRotation().y,
                                           obj.getRotation().x,
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

    btMotionState* motionState = new btDefaultMotionState(transform);

    //put in info struct and create rigid body
    btRigidBody::btRigidBodyConstructionInfo bodyInfo(obj.mass, motionState, shape, inertia);
    btRigidBody* body = new btRigidBody(bodyInfo);
    obj.bulletBody = body;
    body->setUserPointer(&obj);

    body->setRestitution(obj.restitution);
    body->setFriction(obj.friction);
    body->setDamping(obj.damping, body->getAngularDamping());

    //enable callback function
    //body->setCollisionFlags(body->getCollisionFlags() | btCollisionObject::CF_CUSTOM_MATERIAL_CALLBACK);

    m_dynamicsWorld->addRigidBody(body);


    return true;
}

bool BulletPhysics::addTerrain(Terrain& terrain, GameObject& obj)
{
    btTransform transform;
    transform.setIdentity();

    //convert height field data to flip z axis
    convertedTerrainData = new float[terrain.mHeightMap.size()]();
    for(UINT i = 0; i < terrain.terrainSlices; i++)
    {
        memcpy(convertedTerrainData + (long long)terrain.terrainSlices * i,
               &terrain.mHeightMap[0] + (long long)terrain.terrainSlices * ((long long)terrain.terrainSlices - i - 1),
               sizeof(float) * terrain.terrainSlices);
    }

    btHeightfieldTerrainShape* terrainShape = new btHeightfieldTerrainShape(
                                                                   terrain.terrainSlices,
                                                                   terrain.terrainSlices,
                                                                   convertedTerrainData,
                                                                   1, //ignored
                                                                   -terrain.heightScale,
                                                                   terrain.heightScale,
                                                                   1, //y = up axis
                                                                   PHY_FLOAT,
                                                                   true); // does this even do anything
    //terrainShape->setUseDiamondSubdivision(); // not needed?

    float scaling = terrain.terrainSize / terrain.terrainSlices;
    terrainShape->setLocalScaling(btVector3(scaling,1.0f, scaling));
    terrainShape->buildAccelerator();
    

    auto motionState = new btDefaultMotionState(transform);
    btRigidBody::btRigidBodyConstructionInfo bodyInfo(0.0f, motionState, terrainShape, btVector3(0,0,0));
    btRigidBody* body = new btRigidBody(bodyInfo);

    body->setUserPointer(&obj);
    body->setRestitution(1.0f);
    body->setFriction(0.5f);
    body->setRollingFriction(0.1f);
    body->setDamping(0.1f, body->getAngularDamping());

    obj.bulletBody = body;

    m_dynamicsWorld->addRigidBody(body);

    return true;
}

bool BulletPhysics::reset()
{
    for(int i = m_dynamicsWorld->getNumCollisionObjects() - 1; i >= 0; i--)
    {
        m_dynamicsWorld->removeCollisionObject(m_dynamicsWorld->getCollisionObjectArray()[i]);
    }

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
    return new btCapsuleShape(obj.extents.x * 2.0f, obj.extents.y * 2.0f);
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
