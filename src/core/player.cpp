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

    resetCoins();
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

    // Transfer transformation back from bullet object if the object is not static
    if(motionType != ObjectMotionType::Static)
    {
        btTransform t = bulletBody->getWorldTransform();
        //bulletBody->getMotionState()->getWorldTransform(t);
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