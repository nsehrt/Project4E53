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

    objectCBSize = d3dUtil::CalcConstantBufferSize(sizeof(ObjectConstants));

    renderItem = std::move(rItem);

    updateTransforms();
}

void GameObject::update(const GameTime& gt)
{
    if (renderItem->renderType != RenderType::Default) return;
    if (name != "box2")return;
    XMFLOAT3 rot = getRotation();
    rot.y += 0.25f * gt.DeltaTime();
    setRotation(rot);
}

void GameObject::draw(RenderType _renderType, ID3D12Resource* objectCB)
{
    const auto gObjRenderItem = renderItem.get();

    if (gObjRenderItem->renderType != _renderType) return;
    if (!isDrawEnabled) return;

    const auto renderResource = ServiceProvider::getRenderResource();

    D3D12_GPU_VIRTUAL_ADDRESS cachedObjCBAddress = 0;

    for (const auto& gObjMeshes : gObjRenderItem->Model->meshes)
    {
        renderResource->cmdList->IASetVertexBuffers(0, 1, &gObjMeshes.second->VertexBufferView());
        renderResource->cmdList->IASetIndexBuffer(&gObjMeshes.second->IndexBufferView());
        renderResource->cmdList->IASetPrimitiveTopology(gObjRenderItem->PrimitiveType);

        D3D12_GPU_VIRTUAL_ADDRESS objCBAddress = objectCB->GetGPUVirtualAddress() + (long long)gObjRenderItem->ObjCBIndex * objectCBSize;

        /*only if changed*/
        if (cachedObjCBAddress != objCBAddress)
        {
            renderResource->cmdList->SetGraphicsRootConstantBufferView(0, objCBAddress);
            cachedObjCBAddress = objCBAddress;
        }

        renderResource->cmdList->DrawIndexedInstanced(gObjMeshes.second->IndexCount, 1, 0, 0, 0);
    }


}

void GameObject::drawHitbox(RenderType _renderType, ID3D12Resource* objectCB)
{

    const auto gObjRenderItem = renderItem.get();

    if (gObjRenderItem->Model->boundingBoxMesh.get() == nullptr) return;

    if (gObjRenderItem->renderType != _renderType) return;
    if (!isDrawEnabled) return;

    const auto renderResource = ServiceProvider::getRenderResource();


    renderResource->cmdList->IASetVertexBuffers(0, 1, &gObjRenderItem->Model->boundingBoxMesh.get()->VertexBufferView());
    renderResource->cmdList->IASetIndexBuffer(&gObjRenderItem->Model->boundingBoxMesh.get()->IndexBufferView());
    renderResource->cmdList->IASetPrimitiveTopology(gObjRenderItem->PrimitiveType);

    D3D12_GPU_VIRTUAL_ADDRESS objCBAddress = objectCB->GetGPUVirtualAddress() + (long long)gObjRenderItem->ObjCBIndex * objectCBSize;

    renderResource->cmdList->SetGraphicsRootConstantBufferView(0, objCBAddress);

    renderResource->cmdList->DrawIndexedInstanced(gObjRenderItem->Model->boundingBoxMesh.get()->IndexCount, 1, 0, 0, 0);



}

void GameObject::updateTransforms()
{

    /*update transforms for constant buffer*/
    XMStoreFloat4x4(&renderItem->World, XMMatrixRotationRollPitchYawFromVector(XMLoadFloat3(&Rotation)) *
                    XMMatrixScalingFromVector(XMLoadFloat3(&Scale)) *
                    XMMatrixTranslationFromVector(XMLoadFloat3(&Position)));

    /*update hitbox*/
    
    renderItem->Model->boundingBox.Transform(hitBox, Scale.x, XMQuaternionRotationRollPitchYawFromVector(XMLoadFloat3(&Rotation)), XMLoadFloat3(&Position));

    renderItem->NumFramesDirty = gNumFrameResources;

}
