#include "../core/player.h"

#include "../core/level.h"
#include "../util/serviceprovider.h"

Player::Player(const std::string& model) : Character("Player", model, 0, 0)
{
    setIsInFrustum(true);
    setPosition({ 0,0,0 });
    setAnimation(ServiceProvider::getRenderResource()->mAnimations["geo_Walk"].get());
}

void Player::update(const InputSet& input, const GameTime& gt)
{

    auto activeLevel = ServiceProvider::getActiveLevel();



    setPosition({ 0,activeLevel->mTerrain->getHeight(0, 0), 0});

    GameObject::update(gt);


}
