#include "bulletcontroller.h"
#include "../util/serviceprovider.h"
#include "../input/inputmanager.h"
#include "../core/player.h"
#include "../util/mathhelper.h"
#include <cassert>
#include <DirectXMath.h>

using namespace DirectX;

BulletController::BulletController(btRigidBody* body)
{

    rigidBody = body;
    setupBody();
}

void BulletController::updateAction(btCollisionWorld* collisionWorld, btScalar deltaTimeStep)
{
    /*get pointer from service provider*/
    auto player = ServiceProvider::getPlayer();
    auto activeLevel = ServiceProvider::getActiveLevel();
    InputSet& input = ServiceProvider::getInputManager()->getInput();

    /*keep old states*/
    player->previousCState = player->currentCState;
    previousDistanceToGround = distanceToGround;
    previousOnGround = onGround;

    /*read player input*/
    bool pressedJump = input.Pressed(BTN::A);
    bool pressedRun = input.current.trigger[TRG::RIGHT_TRIGGER] > 0.1f;
    bool pressedRoll = input.Pressed(BTN::B);

    XMFLOAT2 inputDirection = { input.current.trigger[TRG::THUMB_LX], input.current.trigger[TRG::THUMB_LY] };
    XMVECTOR inputDirectionV = XMVector2Normalize(XMLoadFloat2(&inputDirection));
    XMStoreFloat2(&inputDirection, inputDirectionV);


    /*ray test to check distance from ground*/
    auto p = player->getPosition();

    /*create start and end point of ray*/
    btVector3 playerCenter = { p.x, p.y + player->extents.y, p.z };
    btVector3 rayEndPoint = { playerCenter.x(), playerCenter.y() - rayLength,  playerCenter.z() };

    btCollisionWorld::ClosestRayResultCallback rayToGroundCast(playerCenter,
                                               rayEndPoint);
    
    collisionWorld->rayTest(playerCenter, rayEndPoint, rayToGroundCast);


    if(rayToGroundCast.hasHit())
    {

        objectUnderPlayer = static_cast<GameObject*>(rayToGroundCast.m_collisionObject->getUserPointer());
        distanceToGround = rayToGroundCast.m_closestHitFraction * rayLength - player->extents.y;

        onGround = distanceToGround <= offGroundThreshold;
        
    }
    else
    {
        objectUnderPlayer = nullptr;
        onGround = false;
        distanceToGround = rayLength - player->extents.y;
    }


    /*actions/states based on current player state/position */
    if(player->previousCState == Character::CharacterState::Idle)
    {
        if(!onGround)
        {
            player->currentCState = Character::CharacterState::Walk;
        }
    }

    /*rotate player in left stick direction*/
    if(inputDirection.x != 0.0f && inputDirection.y != 0.0f)
    {
        float targetRotationY = MathHelper::angleFromXY(inputDirection.y, inputDirection.x) - MathHelper::Pi;
        player->Rotation.y = MathHelper::lerpAngle(player->Rotation.y, targetRotationY, 6.25f * deltaTimeStep);
        rigidBody->setGravity(onGroundGravity);
    }
    else
    {
        player->currentCState = Character::CharacterState::Walk;
        rigidBody->setGravity(onIdleGravity);
    }

 
    //TODO
    const btVector3& current = rigidBody->getLinearVelocity();
    rigidBody->setLinearVelocity({ inputDirection.x * 7.0f, pressedJump ? 10.0f : current.y(), inputDirection.y * 7.0f });
    

}

void BulletController::setupBody()
{
    assert(rigidBody);
    rigidBody->setSleepingThresholds(0.0f, 0.0f);
    rigidBody->setAngularFactor(0.0f);
    rigidBody->setRestitution(0.0f);
    rigidBody->setFriction(0.0f);
    rigidBody->setRollingFriction(0.0f);
    rigidBody->setGravity(onGroundGravity);
}


void BulletController::debugDraw(btIDebugDraw* debugDrawer)
{
}