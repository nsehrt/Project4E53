#include "gameobject.h"

using namespace DirectX;

GameObject::GameObject(const json& objectJson, int index)
{
    /*Name*/
    name = objectJson["Name"];

    /*Transforms*/

    if (exists(objectJson, "Position"))
    {
        Position.x = objectJson["Position"][0];
        Position.y = objectJson["Position"][1];
        Position.z = objectJson["Position"][2];
    }
    else
    {
        Position = XMFLOAT3(0.0f, 0.0f, 0.0f);
    }

    if (exists(objectJson, "Scale"))
    {
        Scale.x = objectJson["Scale"][0];
        Scale.y = objectJson["Scale"][1];
        Scale.z = objectJson["Scale"][2];
    }
    else
    {
        Scale = XMFLOAT3(1.0f, 1.0f, 1.0f);
    }

    if (exists(objectJson, "Rotation"))
    {
        Rotation.x = XMConvertToRadians(objectJson["Rotation"][0]);
        Rotation.y = XMConvertToRadians(objectJson["Rotation"][1]);
        Rotation.z = XMConvertToRadians(objectJson["Rotation"][2]);
    }
    else
    {
        Rotation = XMFLOAT3(0.0f, 0.0f, 0.0f);
    }


    /*Texture Transforms*/

    if (exists(objectJson, "TexTranslation"))
    {
        TextureTranslation.x = objectJson["TexTranslation"][0];
        TextureTranslation.y = objectJson["TexTranslation"][1];
        TextureTranslation.z = objectJson["TexTranslation"][2];
    }
    else
    {
        TextureTranslation = XMFLOAT3(0.0f, 0.0f, 0.0f);
    }

    if (exists(objectJson, "TexRotation"))
    {
        TextureRotation.x = XMConvertToRadians(objectJson["TexRotation"][0]);
        TextureRotation.y = XMConvertToRadians(objectJson["TexRotation"][1]);
        TextureRotation.z = XMConvertToRadians(objectJson["TexRotation"][2]);
    }
    else
    {
        TextureRotation = XMFLOAT3(0.0f, 0.0f, 0.0f);
    }

    if (exists(objectJson, "TexScale"))
    {
        TextureScale.x = objectJson["TexScale"][0];
        TextureScale.y = objectJson["TexScale"][1];
        TextureScale.z = objectJson["TexScale"][2];
    }
    else
    {
        TextureScale = XMFLOAT3(1.0f, 1.0f, 1.0f);
    }

    /*Flags*/
    if (exists(objectJson, "CollisionEnabled"))
    {
        isCollisionEnabled = objectJson["CollisionEnabled"];
    }

    if (exists(objectJson, "DrawEnabled"))
    {
        isDrawEnabled = objectJson["DrawEnabled"];
    }

    if (exists(objectJson, "ShadowEnabled"))
    {
        isShadowEnabled = objectJson["ShadowEnabled"];
    }

    if (exists(objectJson, "FrustumCulled"))
    {
        isFrustumCulled = objectJson["FrustumCulled"];
    }

    if (exists(objectJson, "ShadowForced"))
    {
        isShadowForced = objectJson["ShadowForced"];
    }

    /*RenderItem*/

    auto renderResource = ServiceProvider::getRenderResource();

    auto rItem = std::make_unique<RenderItem>();

    rItem->ObjCBIndex.push_back(index);

    /*check model exists*/
    if (renderResource->mModels.find(objectJson["Model"]) == renderResource->mModels.end())
    {
        if (objectJson["Model"] == "")
        {
            /*invisible wall*/
            isCollisionEnabled = true;
            isDrawEnabled = false;
            isShadowEnabled = false;
            isShadowForced = false;
            rItem->Model = renderResource->mModels["box"].get();
            TextureScale = Scale;
            gameObjectType = GameObjectType::Wall;
        }
        else
        {
            LOG(Severity::Warning, "GameObject " << name << " specified not loaded model " << objectJson["Model"] << "!");

            rItem->Model = renderResource->mModels["box"].get();
        }

    }
    else
    {
        rItem->Model = renderResource->mModels[objectJson["Model"]].get();
    }

    /*check material exists*/
    if (renderResource->mMaterials.find(objectJson["Material"]) == renderResource->mMaterials.end())
    {
        if (gameObjectType == GameObjectType::Wall)
        {
            rItem->Mat = renderResource->mMaterials["invWall"].get();
        }
        else
        {
            LOG(Severity::Warning, "GameObject " << name << " specified not loaded material " << objectJson["Material"] << "!");
            rItem->Mat = renderResource->mMaterials["default"].get();
        }

    }
    else
    {
        rItem->Mat = renderResource->mMaterials[objectJson["Material"]].get();
    }

    /*render type*/
    if (!exists(objectJson, "RenderType"))
    {
        if (gameObjectType == GameObjectType::Wall)
        {
            rItem->renderType = RenderType::DefaultTransparency;
        }
        else
        {
            rItem->renderType = RenderType::Default;
        }
    }
    else
    {
        if (objectJson["RenderType"] == "DefaultAlpha")
        {
            rItem->renderType = RenderType::DefaultAlpha;
            rItem->shadowType = RenderType::ShadowAlpha;
        }
        else if (objectJson["RenderType"] == "DefaultNoNormal")
        {
            rItem->renderType = RenderType::DefaultNoNormal;
        }
        else if (objectJson["RenderType"] == "Debug")
        {
            rItem->renderType = RenderType::Debug;
        }
        else if (objectJson["RenderType"] == "DefaultTransparency")
        {
            rItem->renderType = RenderType::DefaultTransparency;
        }
        else
        {
            rItem->renderType = RenderType::Default;
        }
    }



    objectCBSize = d3dUtil::CalcConstantBufferSize(sizeof(ObjectConstants));

    renderItem = std::move(rItem);

    updateTransforms();
}

GameObject::GameObject()
{
    Position = XMFLOAT3(0.0f, 0.0f, 0.0f);
    Rotation = XMFLOAT3(0.0f, 0.0f, 0.0f);
    Scale = XMFLOAT3(1.0f, 1.0f, 1.0f);

    TextureTranslation = XMFLOAT3(0.0f, 0.0f, 0.0f);
    TextureRotation = XMFLOAT3(0.0f, 0.0f, 0.0f);
    TextureScale = XMFLOAT3(1.0f, 1.0f, 1.0f);

}

GameObject::GameObject(int index)
{
    auto renderResource = ServiceProvider::getRenderResource();

    Position = XMFLOAT3(0.0f, 0.0f, 0.0f);
    Rotation = XMFLOAT3(0.0f, 0.0f, 0.0f);
    Scale = XMFLOAT3(1.0f, 1.0f, 1.0f);

    TextureTranslation = XMFLOAT3(0.0f, 0.0f, 0.0f);
    TextureRotation = XMFLOAT3(0.0f, 0.0f, 0.0f);
    TextureScale = XMFLOAT3(1.0f, 1.0f, 1.0f);

    auto tItem = std::make_unique<RenderItem>();
    tItem->ObjCBIndex.push_back(index);
    tItem->Mat = renderResource->mMaterials["default"].get();
    tItem->Model = renderResource->mModels["box"].get();

    renderItem = std::move(tItem);

}

void GameObject::update(const GameTime& gt)
{

    /*TODO*/
    if (renderItem->renderType != RenderType::Default && renderItem->renderType != RenderType::DefaultNoNormal) return;
    if (name != "box2" && name != "skull2")return;
    XMFLOAT3 rot = getRotation();
    rot.y += 0.25f * gt.DeltaTime();
    setRotation(rot);
    if (name != "skull2") return;
    XMFLOAT3 pos = getPosition();
    pos.x += 2.5f * gt.DeltaTime();
    setPosition(pos);
}

bool GameObject::draw()
{
    const auto gObjRenderItem = renderItem.get();
    const auto objectCB = ServiceProvider::getRenderResource()->getCurrentFrameResource()->ObjectCB->getResource();

    if (gameObjectType == GameObjectType::Wall)
    {
        int i = 1;
    }

    if (!isDrawEnabled &&
        !(gameObjectType == GameObjectType::Wall && ServiceProvider::getSettings()->miscSettings.EditModeEnabled))
    {
        return false;
    }


    /*frustum culling check*/
    if (isFrustumCulled)
    {
        auto cam = ServiceProvider::getActiveCamera();

        XMMATRIX world = XMLoadFloat4x4(&gObjRenderItem->World);
        XMMATRIX invView = XMMatrixInverse(&XMMatrixDeterminant(cam->getView()), cam->getView());

        BoundingFrustum localSpaceFrustum;
        cam->getFrustum().Transform(localSpaceFrustum, invView);

        if (localSpaceFrustum.Contains(hitBox) == DirectX::DISJOINT)
        {
            return false;
        }

    }

    const auto renderResource = ServiceProvider::getRenderResource();

    D3D12_GPU_VIRTUAL_ADDRESS cachedObjCBAddress = 0;

    UINT meshCounter = 0;

    for (const auto& gObjMeshes : gObjRenderItem->Model->meshes)
    {
        renderResource->cmdList->IASetVertexBuffers(0, 1, &gObjMeshes.second->VertexBufferView());
        renderResource->cmdList->IASetIndexBuffer(&gObjMeshes.second->IndexBufferView());
        renderResource->cmdList->IASetPrimitiveTopology(gObjRenderItem->PrimitiveType);

        D3D12_GPU_VIRTUAL_ADDRESS objCBAddress = objectCB->GetGPUVirtualAddress() + 
                            (gObjRenderItem->uniformObjCB ? (long long)gObjRenderItem->ObjCBIndex[0] : (long long)gObjRenderItem->ObjCBIndex[meshCounter]) * objectCBSize;

        /*only if changed*/
        if (cachedObjCBAddress != objCBAddress)
        {
            renderResource->cmdList->SetGraphicsRootConstantBufferView(0, objCBAddress);
            cachedObjCBAddress = objCBAddress;
        }

        renderResource->cmdList->DrawIndexedInstanced(gObjMeshes.second->IndexCount, 1, 0, 0, 0);

        meshCounter++;
    }

    return true;
}

bool GameObject::drawShadow()
{
    const auto gObjRenderItem = renderItem.get();
    const auto objectCB = ServiceProvider::getRenderResource()->getCurrentFrameResource()->ObjectCB->getResource();


    if (!isDrawEnabled || !isShadowEnabled) return false;

    const auto renderResource = ServiceProvider::getRenderResource();

    D3D12_GPU_VIRTUAL_ADDRESS cachedObjCBAddress = 0;

    UINT meshCounter = 0;

    for (const auto& gObjMeshes : gObjRenderItem->Model->meshes)
    {
        renderResource->cmdList->IASetVertexBuffers(0, 1, &gObjMeshes.second->VertexBufferView());
        renderResource->cmdList->IASetIndexBuffer(&gObjMeshes.second->IndexBufferView());
        renderResource->cmdList->IASetPrimitiveTopology(gObjRenderItem->PrimitiveType);

        D3D12_GPU_VIRTUAL_ADDRESS objCBAddress = objectCB->GetGPUVirtualAddress() +
            (gObjRenderItem->uniformObjCB ? (long long)gObjRenderItem->ObjCBIndex[0] : (long long)gObjRenderItem->ObjCBIndex[meshCounter]) * objectCBSize;

        /*only if changed*/
        if (cachedObjCBAddress != objCBAddress)
        {
            renderResource->cmdList->SetGraphicsRootConstantBufferView(0, objCBAddress);
            cachedObjCBAddress = objCBAddress;
        }

        renderResource->cmdList->DrawIndexedInstanced(gObjMeshes.second->IndexCount, 1, 0, 0, 0);

        meshCounter++;
    }

    return true;
}

void GameObject::drawHitbox()
{

    const auto gObjRenderItem = renderItem.get();
    const auto objectCB = ServiceProvider::getRenderResource()->getCurrentFrameResource()->ObjectCB->getResource();


    if (gObjRenderItem->Model->boundingBoxMesh.get() == nullptr) return;

    const auto renderResource = ServiceProvider::getRenderResource();


    renderResource->cmdList->IASetVertexBuffers(0, 1, &gObjRenderItem->Model->boundingBoxMesh.get()->VertexBufferView());
    renderResource->cmdList->IASetIndexBuffer(&gObjRenderItem->Model->boundingBoxMesh.get()->IndexBufferView());
    renderResource->cmdList->IASetPrimitiveTopology(gObjRenderItem->PrimitiveType);

    D3D12_GPU_VIRTUAL_ADDRESS objCBAddress = objectCB->GetGPUVirtualAddress() + (long long)gObjRenderItem->ObjCBIndex[0] * objectCBSize;

    renderResource->cmdList->SetGraphicsRootConstantBufferView(0, objCBAddress);

    renderResource->cmdList->DrawIndexedInstanced(gObjRenderItem->Model->boundingBoxMesh.get()->IndexCount, 1, 0, 0, 0);

}


bool GameObject::intersects(GameObject& obj)
{
    if (!isCollisionEnabled || !obj.isCollisionEnabled)
    {
        return false;
    }

    return hitBox.Intersects(obj.hitBox);
}


bool GameObject::intersects(DirectX::BoundingBox& box)
{
    if (!isCollisionEnabled)
    {
        return false;
    }

    return hitBox.Intersects(box);
}

bool GameObject::intersectsShadowBounds(DirectX::BoundingSphere& sphere)
{
    return hitBox.Intersects(sphere);
}


void GameObject::updateTransforms()
{

    /*update transforms for constant buffer*/
    XMStoreFloat4x4(&renderItem->World, XMMatrixScalingFromVector(XMLoadFloat3(&Scale)) *
                    XMMatrixRotationRollPitchYawFromVector(XMLoadFloat3(&Rotation)) *
                    XMMatrixTranslationFromVector(XMLoadFloat3(&Position)));

    XMStoreFloat4x4(&renderItem->TexTransform, XMMatrixScalingFromVector(XMLoadFloat3(&TextureScale)) *
                    XMMatrixRotationRollPitchYawFromVector(XMLoadFloat3(&TextureRotation)) *
                    XMMatrixTranslationFromVector(XMLoadFloat3(&TextureTranslation)));

    /*update hitbox*/
    renderItem->Model->boundingBox.Transform(hitBox, XMLoadFloat4x4(&renderItem->World));

    renderItem->NumFramesDirty = gNumFrameResources;

}