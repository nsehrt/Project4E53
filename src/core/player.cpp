#include "../core/player.h"

#include "../core/level.h"
#include "../util/serviceprovider.h"

using namespace DirectX;

Player::Player(const std::string& model) : Character("Player", model, 0, 0)
{
    setIsInFrustum(true);
    setPosition({ 0,0,0 });
    setAnimation(ServiceProvider::getRenderResource()->mAnimations["geo_Walk"].get());
}

void Player::update(const InputSet& input, const GameTime& gt)
{

    auto activeLevel = ServiceProvider::getActiveLevel();

    /*--- player movement*/
    
    /*rotate player in left stick direction*/

    XMFLOAT2 inputVector = { input.current.trigger[TRG::THUMB_LX], input.current.trigger[TRG::THUMB_LY] };
    XMFLOAT2 inputDirection;

    XMVECTOR inputDirectionV = XMVector2Normalize(XMLoadFloat2(&inputVector));

    XMStoreFloat2(&inputDirection, inputDirectionV);

    if (inputDirection.x != 0.0f && inputDirection.y != 0.0f)
    {
        setRotation({ 0, std::atan2f(inputDirection.x, inputDirection.y) - XM_PI,0 });
    }
    
    /*calculate movement speed and apply it*/
    auto inputMagnitudeV = XMVector2Length(inputDirectionV);
    float inputMagnitude;
    XMStoreFloat(&inputMagnitude, inputMagnitudeV);

    float speed = walkSpeed * inputMagnitude;

    //

    auto pPos = getPosition();

    pPos.x += inputDirection.x * walkSpeed * gt.DeltaTime();
    pPos.z += inputDirection.y * walkSpeed * gt.DeltaTime();


    /*players y position to terrain height*/
    
    setPosition({ pPos.x, activeLevel->mTerrain->getHeight(pPos.x, pPos.z), pPos.z});

    GameObject::update(gt);


}
