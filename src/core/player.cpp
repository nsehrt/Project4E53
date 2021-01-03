#include "../core/player.h"
#include "../physics/bulletcontroller.h"
#include "../util/collisiondatabase.h"
#include "../core/level.h"
#include "../util/serviceprovider.h"

using namespace DirectX;

Player::Player(const std::string& model) : Character("Player", model, 0, 0)
{
    setAnimation(SP_ANIM("geo_Idle"));
    isFrustumCulled = false;

    extents = ServiceProvider::getCollisionDatabase()->getExtents("player_model");
}

void Player::stickToTerrain()
{
    const auto pos = getPosition();
    setPosition({ pos.x,
                ServiceProvider::getActiveLevel()->mTerrain->getHeight(pos.x, pos.z) + extents.y,
                pos.z });
}

void Player::update(const GameTime& gt)
{

    //TODO: update animation code here based on speed and action performed etc.

//    if(running)
//    {
//        currentSpeed = runSpeed;
//
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
//
//    currentSpeed = currentSpeed * inputMagnitude;
//
//    /*apply to calculated speed to the player position using the direction the player faces*/
//    auto pPos = getPosition();
//    auto pRot = MathHelper::vectorFromAngle(getRotation().y + XM_PIDIV2);
//
//    pPos.x += pRot.x * currentSpeed * gt.DeltaTime();
//    pPos.z += pRot.z * currentSpeed * gt.DeltaTime();
//
//    projectedPosition = pPos;
//}
//    else
//    {
//    /*reset idle timer when no input (standing still)*/
//    currentCState = CharacterState::Idle;
//
//    if(previousCState != CharacterState::Idle)
//    {
//        setAnimation(SP_ANIM("geo_Idle"), true);
//    }
//
//    timeIdle += gt.DeltaTime();
//    timeMoving = 0.0f;
//
//    if(timeIdle > SP_ANIM("geo_Idle")->getEndTime() * 3.0f)
//    {
//        setAnimation(SP_ANIM("geo_Idle2"), false);
//        timeIdle = 0.0f;
//    }
//    

    // Transfer transformation back from bullet object if the object is not static
    if(motionType != ObjectMotionType::Static)
    {
        btTransform t;
        bulletBody->getMotionState()->getWorldTransform(t);
        XMStoreFloat4x4(&rotationQuat, XMMatrixRotationRollPitchYawFromVector(XMLoadFloat3(&Rotation)));
        Position = { t.getOrigin().x(), t.getOrigin().y() - height, t.getOrigin().z() };

        updateTransforms();
    }

    // update animation for skinned objects
    if(gameObjectType == ObjectType::Skinned)
    {
        if(renderItem->currentClip != nullptr)
        {
            renderItem->animationTimer += gt.DeltaTime() * animationTimeScale;

            if(renderItem->animationTimer >= renderItem->currentClip->getEndTime())
            {
                renderItem->animationTimer = fmod(renderItem->animationTimer, renderItem->currentClip->getEndTime());
            }

            if(renderItem->animationTimer <= renderItem->currentClip->getStartTime())
            {
                renderItem->animationTimer = renderItem->currentClip->getEndTime() - fmod(renderItem->animationTimer, renderItem->currentClip->getEndTime());
            }
        }

        if(currentlyInFrustum || !isFrustumCulled)
            renderItem->skinnedModel->calculateFinalTransforms(renderItem->currentClip, renderItem->finalTransforms, renderItem->animationTimer);
    }
}