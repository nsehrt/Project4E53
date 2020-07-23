#include "../core/character.h"
#include "../util/serviceprovider.h"

Character::Character(const std::string& name, const std::string& model, int index, int skinnedIndex) : GameObject(name, index, skinnedIndex)
{
    auto renderResource = ServiceProvider::getRenderResource();

    if (!Helper::keyInMap(renderResource->mSkinnedModels, model))
    {
        LOG(Severity::Critical, "Can not use inexistant model " << model << "!");
    }

    makeDynamic(renderResource->mSkinnedModels[model].get(), skinnedIndex);

}

void Character::update(const GameTime& gt)
{



}