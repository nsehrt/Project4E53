#include "gameobject.h"

using namespace DirectX;

GameObject::GameObject(const json& objectJson, int index)
{
    /*Name*/
    name = objectJson["Name"];

    /*RenderItem*/

    auto rItem = std::make_unique<RenderItem>();

    rItem->ObjCBIndex = index;

    renderItem = std::move(rItem);

    /*Transforms*/
    Position.x = objectJson["Position"][0];
    Position.y = objectJson["Position"][1];
    Position.z = objectJson["Position"][2];

    Scale.x = objectJson["Scale"][0];
    Scale.y = objectJson["Scale"][1];
    Scale.z = objectJson["Scale"][2];

    Rotation.x = objectJson["Rotation"][0];
    Rotation.y = objectJson["Rotation"][1];
    Rotation.z = objectJson["Rotation"][2];

    /*Flags*/
    isCollisionEnabled = objectJson["CollisionEnabled"];
    isDrawEnabled = objectJson["DrawEnabled"];
    isShadowEnabled = objectJson["ShadowEnabled"];
}
