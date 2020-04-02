#include "gameobject.h"

using namespace DirectX;

GameObject::GameObject(const json& objectJson, int index)
{
    /*Name*/
    name = objectJson["Name"];

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


    /*RenderItem*/

    auto renderResource = ServiceProvider::getRenderResource();

    auto rItem = std::make_unique<RenderItem>();

    rItem->ObjCBIndex = index;

    XMMATRIX translationMatrix = XMMatrixTranslation(Position.x, Position.y, Position.z);
    XMMATRIX scaleMatrix = XMMatrixScaling(Scale.x, Scale.y, Scale.z);
    XMMATRIX rotationMatrix = XMMatrixRotationRollPitchYaw(Rotation.x, Rotation.y, Rotation.z);

    XMStoreFloat4x4(&rItem->World, rotationMatrix * scaleMatrix * translationMatrix);
    XMStoreFloat4x4(&rItem->TexTransform, XMMatrixScaling(1.0f,1.0f,1.0f));

    /*check model exists*/
    if (renderResource->mModels.find(objectJson["Model"]) == renderResource->mModels.end())
    {
        LOG(Severity::Warning, "GameObject " << name << " specified not loaded model " << objectJson["Model"] << "!");

        rItem->Model = renderResource->mModels["box"].get();
    }
    else
    {
        rItem->Model = renderResource->mModels[objectJson["Model"]].get();
    }

    /*check material exists*/
    if (renderResource->mMaterials.find(objectJson["Material"]) == renderResource->mMaterials.end())
    {
        LOG(Severity::Warning, "GameObject " << name << " specified not loaded material " << objectJson["Material"] << "!");

        rItem->Mat = renderResource->mMaterials["default"].get();
    }
    else
    {
        rItem->Mat = renderResource->mMaterials[objectJson["Material"]].get();
    }

    /*render type*/
    if (objectJson["RenderType"] == "DefaultAlpha")
    {
        rItem->renderType = RenderType::DefaultAlpha;
    }
    else
    {
        rItem->renderType = RenderType::Default;
    }

    renderItem = std::move(rItem);


}

void GameObject::draw()
{
    //if (!isDrawEnabled) return;

    //const auto gObjRenderItem = renderItem.get();

    //if (gObjRenderItem->renderType == RenderType::Sky) continue;

    //for (const auto& gObjMeshes : gObjRenderItem->Model->meshes)
    //{
    //    renderResource->cmdList->IASetVertexBuffers(0, 1, &gObjMeshes.second->VertexBufferView());
    //    renderResource->cmdList->IASetIndexBuffer(&gObjMeshes.second->IndexBufferView());
    //    renderResource->cmdList->IASetPrimitiveTopology(gObjRenderItem->PrimitiveType);

    //    D3D12_GPU_VIRTUAL_ADDRESS objCBAddress = objectCB->GetGPUVirtualAddress() + (long long)gObjRenderItem->ObjCBIndex * objCBByteSize;

    //    /*only if changed*/
    //    renderResource->cmdList->SetGraphicsRootConstantBufferView(0, objCBAddress);

    //    renderResource->cmdList->DrawIndexedInstanced(gObjMeshes.second->IndexCount, 1, 0, 0, 0);
    //}


}
