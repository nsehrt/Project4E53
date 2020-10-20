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
    auto player = ServiceProvider::getPlayer();
    auto activeLevel = ServiceProvider::getActiveLevel();
    InputSet& input = ServiceProvider::getInputManager()->getInput();

    GroundCheck groundCheck(this, collisionWorld);

    /*--- player movement*/
    player->previousCState = player->currentCState;

    bool pressedJump = input.Pressed(BTN::A);
    bool pressedRun = input.current.trigger[TRG::RIGHT_TRIGGER] > 0.1f;
    bool pressedRoll = input.Pressed(BTN::B);

    XMFLOAT2 inputVector = { input.current.trigger[TRG::THUMB_LX], input.current.trigger[TRG::THUMB_LY] };
    XMFLOAT2 inputDirection{};
    XMVECTOR inputDirectionV = XMVector2Normalize(XMLoadFloat2(&inputVector));
    XMStoreFloat2(&inputDirection, inputDirectionV);

    /*--- ray test to check distance from ground*/
    auto p = player->getPosition();
    btVector3 playerCenter = { p.x, p.y + player->extents.y, p.z };
    btVector3 rayEndPoint = playerCenter;
    rayEndPoint.setY(rayEndPoint.y() - rayLength);
    btCollisionWorld::ClosestRayResultCallback rayToGroundCast(playerCenter,
                                               rayEndPoint);
    
    collisionWorld->rayTest(playerCenter, rayEndPoint, rayToGroundCast);

    if(rayToGroundCast.hasHit())
    {
        objectUnderPlayer = static_cast<GameObject*>(rayToGroundCast.m_collisionObject->getUserPointer());

        /*check if player is standing on ground*/
        float distanceToGround = rayToGroundCast.m_closestHitFraction * rayLength - player->extents.y;
        onGround = false;

        auto t = player->bulletBody->getWorldTransform().getOrigin();
        //t.setY(t.y() - (distanceToGround - player->extents.y - MathHelper::Epsilon));
        //player->bulletBody->getWorldTransform().setOrigin(t);

        if(maxDistanceFromGround < distanceToGround)
        {
            maxDistanceFromGround = distanceToGround;
            std::cout << "Max distance from ground: " << maxDistanceFromGround << "\n";
        }

        if( (distanceToGround) < slopeThresholdPerSecond * deltaTimeStep)
        {
            onGround = true;
        }
        //LOG(Severity::Debug, distanceToGround);
    }

    //TODO delete
    player->currentCState = Character::CharacterState::Run;

    /*rotate player in left stick direction*/
    if(inputDirection.x != 0.0f && inputDirection.y != 0.0f)
    {
        float targetRotationY = MathHelper::angleFromXY(inputDirection.y, inputDirection.x) - MathHelper::Pi;
        player->Rotation.y = MathHelper::lerpAngle(player->Rotation.y, targetRotationY, 6.25f * deltaTimeStep);
    }

    //float targetRotation = MathHelper::angleFromXY(inputDirection.y, inputDirection.x) - MathHelper::Pi;//std::atan2f(inputDirection.x, inputDirection.y) - XM_PI;
    //player->setRotation({ 0, MathHelper::lerpAngle(player->getRotation().y, targetRotation, turnSmoothTime * deltaTimeStep) ,0 }, false);


    //if(inputDirection.x != 0.0f && inputDirection.y != 0.0f)
    //{
    //    /*reset idle timer while player is active*/
    //    timeIdle = 0.0f;
    //    timeMoving += gt.DeltaTime();

    //    /*do not instantly rotate to target rotation (input direction) but lerp there over time*/



    //    /*calculate movement speed*/

    //    auto inputMagnitudeV = XMVector2Length(XMLoadFloat2(&inputVector));
    //    float inputMagnitude{};
    //    XMStoreFloat(&inputMagnitude, inputMagnitudeV);

    //    /*use unnormalized inputMagnitude to have finer control over movement speed*/
    //    float currentSpeed = walkSpeed;

    //    if(running)
    //    {
    //        currentSpeed = runSpeed;

    //        currentCState = CharacterState::Run;
    //        if(previousCState != CharacterState::Run)
    //        {
    //            setAnimation(SP_ANIM("geo_Run"), true);
    //        }
    //    }
    //    else
    //    {
    //        currentCState = CharacterState::Walk;
    //        if(previousCState != CharacterState::Walk)
    //        {
    //            setAnimation(SP_ANIM("geo_Walk"), true);
    //        }
    //    }

    //    currentSpeed = currentSpeed * inputMagnitude;

    //    /*apply to calculated speed to the player position using the direction the player faces*/
    //    auto pPos = getPosition();
    //    auto pRot = MathHelper::vectorFromAngle(getRotation().y + XM_PIDIV2);

    //    pPos.x += pRot.x * currentSpeed * gt.DeltaTime();
    //    pPos.z += pRot.z * currentSpeed * gt.DeltaTime();

    //    projectedPosition = pPos;

    //}
    //else
    //{
    //    /*reset idle timer when no input (standing still)*/
    //    currentCState = CharacterState::Idle;

    //    if(previousCState != CharacterState::Idle)
    //    {
    //        setAnimation(SP_ANIM("geo_Idle"), true);
    //    }

    //    timeIdle += gt.DeltaTime();
    //    timeMoving = 0.0f;

    //    if(timeIdle > SP_ANIM("geo_Idle")->getEndTime() * 3.0f)
    //    {
    //        setAnimation(SP_ANIM("geo_Idle2"), false);
    //        timeIdle = 0.0f;
    //    }

    //}

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
    rigidBody->setGravity({ 0.0f,-50.0f,0.0f });
    bodyGravity = rigidBody->getGravity().y();
}


void BulletController::debugDraw(btIDebugDraw* debugDrawer)
{
}


btScalar GroundCheck::addSingleResult(btManifoldPoint& cp, const btCollisionObjectWrapper* colObj0, int partId0, int index0, const btCollisionObjectWrapper* colObj1, int partId1, int index1)
{
    return 0;
}

void GroundCheck::checkGround(const btManifoldPoint& cp)
{
}
