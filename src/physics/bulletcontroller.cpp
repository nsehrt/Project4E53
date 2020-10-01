#include "bulletcontroller.h"
#include <btBulletDynamicsCommon.h>
#include <cassert>

BulletController::BulletController(btRigidBody* body)
{

    rigidBody = body;

    setupBody();
}

void BulletController::updateAction(btCollisionWorld* collisionWorld, btScalar deltaTimeStep)
{



}

void BulletController::debugDraw(btIDebugDraw* debugDrawer)
{
}

void BulletController::setupBody()
{
    assert(rigidBody);
    rigidBody->setSleepingThresholds(0.0f, 0.0f);
    rigidBody->setAngularFactor(0.0f);
    bodyGravity = rigidBody->getGravity().y();
}
