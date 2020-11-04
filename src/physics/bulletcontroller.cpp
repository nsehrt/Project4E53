#include "bulletcontroller.h"
#include "../util/serviceprovider.h"
#include "../input/inputmanager.h"
#include <cassert>

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

    /*keep old states*/
    player->previousCState = player->currentCState;
    previousDistanceToGround = distanceToGround;
    previousOnGround = onGround;
    previousVelocity = velocity;
    jumpedThisFrame = false;

    timeInCurrentState += deltaTimeStep;


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

    /*calculate intended velocity purely based on current input*/
    XMFLOAT2 intendedVelocity{};
    XMStoreFloat2(&intendedVelocity, XMVectorScale(XMLoadFloat2(&inputDirection), inputMagnitude));
    
    if(!pressedRun)
    {
        XMStoreFloat2(&intendedVelocity, XMVectorScale(XMLoadFloat2(&intendedVelocity), walkSpeed));
    }
    else
    {
        XMStoreFloat2(&intendedVelocity, XMVectorScale(XMLoadFloat2(&intendedVelocity), runSpeed));
    }
    
    /* figure out the next state */
    if(player->currentCState == CharacterState::Ground)
    {

        // no ground under player -> player must be falling
        if(!onGround)
        {
            setState(CharacterState::Fall);
            rigidBody->setGravity(inAirGravity);
        }

        // pressed jump is valid, initiate jump
        if(pressedJump)
        {
            setState(CharacterState::Jump);
            jumpedThisFrame = true;
            rigidBody->setGravity(inAirGravity);
        }

    }
    else if(player->currentCState == CharacterState::Jump)
    {

        if(player->currentCState == CharacterState::Jump)
        {

        }

        if(onGround && timeInCurrentState > minimumJumpTime)
        {
            setState(CharacterState::Ground);
            rigidBody->setGravity(onGroundGravity);
        }

    }
    else if(player->currentCState == CharacterState::Fall)
    {

        if(onGround)
        {
            setState(CharacterState::Ground);
            rigidBody->setGravity(onGroundGravity);
        }

        // pressed jump is valid if grace period not over
        if(pressedJump && timeInCurrentState < fallJumpGracePeriod)
        {
            setState(CharacterState::Jump);
            jumpedThisFrame = true;
            rigidBody->setGravity(inAirGravity);
        }

    }


    /*controls according to current state*/
    const btVector3& currentVelocity = rigidBody->getLinearVelocity();

    

    if(player->currentCState == CharacterState::Ground)
    {
        /*rotate player in left stick direction*/
        if(inputDirection.x != 0.0f && inputDirection.y != 0.0f)
        {
            float targetRotationY = MathHelper::angleFromVector2Centered(inputDirection);
            //targetRotationY = MathHelper::lerpAngle(player->Rotation.y, targetRotationY, turnSmoothTime * deltaTimeStep);
            player->Rotation.y = targetRotationY;

            std::cout << XMConvertToDegrees(player->Rotation.y) << "\n";

            velocity = intendedVelocity;
            rigidBody->setLinearVelocity({ velocity.x, currentVelocity.y(), velocity.y });
        }
        else
        {
            velocity = intendedVelocity;
            rigidBody->setLinearVelocity({ velocity.x, currentVelocity.y(), velocity.y });
        }


    }
    else if(player->currentCState == CharacterState::Jump ||
            player->currentCState == CharacterState::Fall)
    {

        if(jumpedThisFrame)
        {
            rigidBody->setLinearVelocity({ intendedVelocity.x, jumpUpVelocity, intendedVelocity.y });
        }
        else
        {

        }

    }


    resetMovementParameters();

}

void BulletController::setupBody()
{
    assert(rigidBody);
    rigidBody->setSleepingThresholds(0.0f, 0.0f);
    rigidBody->setAngularFactor(0.0f);
    rigidBody->setRestitution(0.0f);
    rigidBody->setFriction(0.0f);
    rigidBody->setRollingFriction(0.0f);
    rigidBody->setDamping(0.3f, 0.0f);
    rigidBody->setGravity(onGroundGravity);
}

void BulletController::setState(const CharacterState state)
{
    std::cout << "State changed to " << state << " after " << timeInCurrentState << "s.\n";
    ServiceProvider::getPlayer()->currentCState = state;
    timeInCurrentState = 0.0f;
}

void BulletController::resetMovementParameters()
{
    pressedJump = false;
    pressedRoll = false;
    pressedRun = false;
}


void BulletController::debugDraw(btIDebugDraw* debugDrawer)
{
}

void BulletController::jump()
{
    pressedJump = true;
}

void BulletController::run(bool value)
{
    pressedRun = true;
}

void BulletController::setMovement(const XMFLOAT2& direction, const float magnitude)
{
    inputDirection = direction;
    inputMagnitude = magnitude;
}
