#include "../core/character.h"
#include "../physics/bulletcontroller.h"
#include "../util/collisiondatabase.h"
#include "../util/serviceprovider.h"

Character::Character(const std::string& name, const std::string& model, int index, int skinnedIndex) : GameObject(name, index, skinnedIndex)
{
    auto renderResource = ServiceProvider::getRenderResource();

    if (!Helper::keyInMap(renderResource->mSkinnedModels, model))
    {
        LOG(Severity::Critical, "Can not use inexistant model " << model << "!");
    }

    makeDynamic(renderResource->mSkinnedModels[model].get(), skinnedIndex);

    //physic properties
    shapeType = CAPSULE_SHAPE_PROXYTYPE;
    extents = ServiceProvider::getCollisionDatabase()->getExtents(model);
    mass = 40.0f;
    restitution = 0.0f;
    motionType = ObjectMotionType::Dynamic;

}

void Character::setupController()
{

    charController = std::make_unique<BulletController>(bulletBody);

}

BulletController* Character::getController() const
{
    return charController.get();
}

void Character::update(const GameTime& gt)
{



}