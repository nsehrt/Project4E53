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
    prevMovementVector = movementVector;
    previouseVelocity = currentVelocity;
    previousIsIdle = isIdle;
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
    intendedVelocity = std::abs(inputMagnitude);
    
    if(pressedRun)
    {
        intendedVelocity *= runSpeed;
    }
    else
    {
        intendedVelocity *= walkSpeed;
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
        else if(isIdle)
        {
            rigidBody->setGravity(onIdleGravity);
        }
    }
    else if(player->currentCState == CharacterState::Jump)
    {

        if(onGround && timeInCurrentState > minimumJumpTime)
        {
            setState(CharacterState::Ground);
            rigidBody->setGravity(onGroundGravity);
        }

    }
    else if(player->currentCState == CharacterState::Fall)
    {
        // pressed jump is valid if grace period not over
        if(pressedJump && timeInCurrentState < fallJumpGracePeriod)
        {
            setState(CharacterState::Jump);
            jumpedThisFrame = true;
            rigidBody->setGravity(inAirGravity);
        }

        rigidBody->setGravity(inAirGravity);
        if(onGround)
        {
            setState(CharacterState::Ground);
            rigidBody->setGravity(onGroundGravity);
        }

    }


    /*controls according to current state*/
    const btVector3& currentVelocityVec = rigidBody->getLinearVelocity();

    /*interpolate current velocity using target velocity and current velocity*/

    if(currentVelocity < intendedVelocity)
    {
        currentVelocity += moveIncreaseConstant * deltaTimeStep;
        if(currentVelocity > intendedVelocity)
        {
            currentVelocity = intendedVelocity;
        }
    }
    else
    {
        currentVelocity -= moveDecreaseConstant * deltaTimeStep;
        if(currentVelocity < intendedVelocity)
        {
            currentVelocity = intendedVelocity;
        }
    }

    if(player->currentCState == CharacterState::Ground)
    {

        /*rotate player in left stick direction*/
        if(inputDirection.x != 0.0f && inputDirection.y != 0.0f)
        {

            /*interpolate between target rotation vector and current rotation vector using the turnSmoothConstant and apply it to model*/
            float targetRotationY = MathHelper::angleFromVector2Centered({ -inputDirection.x, inputDirection.y });
            targetRotationY = MathHelper::lerpAngle(player->Rotation.y, targetRotationY, turnSmoothConstant * deltaTimeStep);
            player->Rotation.y = targetRotationY;

            /*rebuild the targetRotationVector from float*/
            auto targetRotationVec2 = MathHelper::vector2FromAngleCentered(targetRotationY);
            targetRotationVec2.x *= -1.0f;

            /*calculate movement vector using rotation and velocity*/
            XMStoreFloat2(&movementVector, XMVectorScale(XMLoadFloat2(&targetRotationVec2), currentVelocity));
            rigidBody->setLinearVelocity({ movementVector.x, currentVelocityVec.y(), movementVector.y });
        }
        else
        {
            auto targetRotationVec2 = MathHelper::vector2FromAngleCentered(player->Rotation.y);
            targetRotationVec2.x *= -1.0f;
            XMStoreFloat2(&movementVector, XMVectorScale(XMLoadFloat2(&targetRotationVec2), currentVelocity));
            rigidBody->setLinearVelocity({ movementVector.x, 0.0f, movementVector.y });
        }


    }
    else if(player->currentCState == CharacterState::Jump ||
            player->currentCState == CharacterState::Fall)
    {

        if(jumpedThisFrame)
        {
            rigidBody->setLinearVelocity({ movementVector.x, jumpUpVelocity, movementVector.y });
        }
        else
        {

        }

    }



    //update player model animation according to the gathered information

    if(currentVelocity < MathHelper::Epsilon)
    {
        if(isIdle)
        {
            timeIdle += deltaTimeStep;
        }
        else
        {
            isIdle = true;
            timeIdle = deltaTimeStep;
            player->setAnimation(SP_ANIM("geo_Idle"));
        }
    }
    else
    {
        bool keepAnimTime = false;
        if(!isIdle)
        {
            keepAnimTime = true;
        }

        if(pressedRun)
        {
            player->setAnimation(SP_ANIM("geo_Run"), keepAnimTime);
        }
        else
        {
            player->setAnimation(SP_ANIM("geo_Walk"), keepAnimTime);
        }

        isIdle = false;
    }
    

    if(movementReset)
    {
        rigidBody->setLinearVelocity({ 0.f, rigidBody->getLinearVelocity().y(), 0.f });
        player->Rotation.y = 0.0f;
        movementReset = false;
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
    //disabled
    //pressedJump = true;
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
