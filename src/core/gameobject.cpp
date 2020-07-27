#include "gameobject.h"
#include "../util/serviceprovider.h"


using namespace DirectX;



GameObject::GameObject(const json& objectJson, int index, int skinnedIndex)
{
    /*Name*/
    Name = objectJson["Name"];

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

    /*RenderItem*/

    auto renderResource = ServiceProvider::getRenderResource();

    auto rItem = std::make_unique<RenderItem>();

    /*check model exists*/
    if (renderResource->mModels.find(objectJson["Model"]) == renderResource->mModels.end())
    {
        if (objectJson["Model"] == "")
        {
            /*invisible wall*/
            isCollisionEnabled = true;
            isDrawEnabled = false;
            isShadowEnabled = false;
            rItem->staticModel = renderResource->mModels["box"].get();
            TextureScale = Scale;
            gameObjectType = GameObjectType::Wall;
        }
        else
        {
            LOG(Severity::Warning, "GameObject " << Name << " specified not loaded model " << objectJson["Model"] << "!");

            rItem->staticModel = renderResource->mModels["box"].get();
        }
    }
    else
    {
        rItem->staticModel = renderResource->mModels[objectJson["Model"]].get();
    }

    /*check material exists*/
    if (exists(objectJson, "Material"))
    {
        if (renderResource->mMaterials.find(objectJson["Material"]) == renderResource->mMaterials.end())
        {
            if (gameObjectType == GameObjectType::Wall)
            {
                rItem->MaterialOverwrite = renderResource->mMaterials["invWall"].get();
            }
            else
            {
                LOG(Severity::Warning, "GameObject " << Name << " specified not loaded material " << objectJson["Material"] << "!");
                rItem->MaterialOverwrite = renderResource->mMaterials["default"].get();
            }
        }
        else
        {
            rItem->MaterialOverwrite = renderResource->mMaterials[objectJson["Material"]].get();
        }
    }

    ASSERT(rItem->staticModel->meshes.size() < 5);

    for (int i = 0; i < 4; i++)//rItem->Model->meshes.size(); i++)
    {
        rItem->ObjCBIndex.push_back(index + i);
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
            rItem->shadowType = ShadowRenderType::ShadowAlpha;
        }
        else if (objectJson["RenderType"] == "DefaultNoNormal")
        {
            rItem->renderType = RenderType::DefaultNoNormal;
        }
        else if (objectJson["RenderType"] == "Debug")
        {
            rItem->renderType = RenderType::Default;
        }
        else if (objectJson["RenderType"] == "DefaultTransparency")
        {
            rItem->renderType = RenderType::DefaultTransparency;
            rItem->shadowType = ShadowRenderType::ShadowAlpha;
        }
        else if (objectJson["RenderType"] == "NoCullNoNormal")
        {
            rItem->renderType = RenderType::NoCullNoNormal;
            rItem->shadowType = ShadowRenderType::ShadowAlpha;
        }
        else
        {
            rItem->renderType = RenderType::Default;
        }
    }

    objectCBSize = d3dUtil::CalcConstantBufferSize(sizeof(ObjectConstants));
    skinnedCBSize = d3dUtil::CalcConstantBufferSize(sizeof(SkinnedConstants));

    renderItem = std::move(rItem);

    /*set collider properties*/
    collider.setBaseBoxes(renderItem->getModel()->baseModelBox);
    setColliderProperties(GameCollider::GameObjectCollider::OBB, renderItem->getModel()->baseModelBox.Center, renderItem->getModel()->baseModelBox.Extents);

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

    objectCBSize = d3dUtil::CalcConstantBufferSize(sizeof(ObjectConstants));
    skinnedCBSize = d3dUtil::CalcConstantBufferSize(sizeof(SkinnedConstants));
}

GameObject::GameObject(const std::string& name, int index, int skinnedIndex)
{
    auto renderResource = ServiceProvider::getRenderResource();

    Name = name;

    Position = XMFLOAT3(0.0f, 0.0f, 0.0f);
    Rotation = XMFLOAT3(0.0f, 0.0f, 0.0f);
    Scale = XMFLOAT3(1.0f, 1.0f, 1.0f);

    TextureTranslation = XMFLOAT3(0.0f, 0.0f, 0.0f);
    TextureRotation = XMFLOAT3(0.0f, 0.0f, 0.0f);
    TextureScale = XMFLOAT3(1.0f, 1.0f, 1.0f);

    auto tItem = std::make_unique<RenderItem>();
    tItem->ObjCBIndex.push_back(index);
    tItem->ObjCBIndex.push_back(index); 
    tItem->ObjCBIndex.push_back(index); 
    tItem->ObjCBIndex.push_back(index);

    if (skinnedIndex == -1)
    {
        tItem->staticModel = renderResource->mModels["box"].get();
        tItem->MaterialOverwrite = renderResource->mMaterials["default"].get();
    }
    else
    {
        gameObjectType = GameObjectType::Dynamic;
        tItem->renderType = RenderType::SkinnedDefault;
        tItem->shadowType = ShadowRenderType::ShadowSkinned;
        tItem->SkinnedCBIndex = skinnedIndex;
    }
    

    renderItem = std::move(tItem);

    objectCBSize = d3dUtil::CalcConstantBufferSize(sizeof(ObjectConstants));
    skinnedCBSize = d3dUtil::CalcConstantBufferSize(sizeof(SkinnedConstants));
}



/***UDPATE***/
void GameObject::update(const GameTime& gt)
{
    if (gameObjectType == GameObjectType::Dynamic)
    {
        if (renderItem->currentClip != nullptr)
        {
            renderItem->animationTimer += gt.DeltaTime() * animationTimeScale;

            if (renderItem->animationTimer >= renderItem->currentClip->getEndTime())
            {
                renderItem->animationTimer = fmod(renderItem->animationTimer, renderItem->currentClip->getEndTime());
            }

            if (renderItem->animationTimer <= renderItem->currentClip->getStartTime())
            {
                renderItem->animationTimer = renderItem->currentClip->getEndTime() - fmod(renderItem->animationTimer, renderItem->currentClip->getEndTime());
            }

        }

        if (currentlyInFrustum || !isFrustumCulled)
            renderItem->skinnedModel->calculateFinalTransforms(renderItem->currentClip, renderItem->finalTransforms, renderItem->animationTimer);

    }


}

bool GameObject::draw() const
{
    const auto gObjRenderItem = renderItem.get();
    const auto objectCB = ServiceProvider::getRenderResource()->getCurrentFrameResource()->ObjectCB->getResource();

    if (gameObjectType == GameObjectType::Debug) return false;

    if (!isDrawEnabled &&
        !(gameObjectType == GameObjectType::Wall && ServiceProvider::getSettings()->miscSettings.EditModeEnabled))
    {
        return false;
    }

    if (!currentlyInFrustum && isFrustumCulled) return false;

    const auto renderResource = ServiceProvider::getRenderResource();

    D3D12_GPU_VIRTUAL_ADDRESS cachedObjCBAddress = 0;

    UINT meshCounter = 0;

    for (const auto& gObjMeshes : gObjRenderItem->getModel()->meshes)
    {
        renderResource->cmdList->IASetVertexBuffers(0, 1, &gObjMeshes->VertexBufferView());
        renderResource->cmdList->IASetIndexBuffer(&gObjMeshes->IndexBufferView());
        renderResource->cmdList->IASetPrimitiveTopology(gObjRenderItem->PrimitiveType);

        D3D12_GPU_VIRTUAL_ADDRESS objCBAddress = objectCB->GetGPUVirtualAddress() + (long long)gObjRenderItem->ObjCBIndex[meshCounter] * objectCBSize;

        /*only if changed*/
        if (cachedObjCBAddress != objCBAddress)
        {
            renderResource->cmdList->SetGraphicsRootConstantBufferView(0, objCBAddress);
            cachedObjCBAddress = objCBAddress;
        }

        if (gObjRenderItem->isSkinned())
        {
            D3D12_GPU_VIRTUAL_ADDRESS skinnedCBAddress = ServiceProvider::getRenderResource()->getCurrentFrameResource()->SkinnedCB->getResource()->GetGPUVirtualAddress() + (long long)gObjRenderItem->SkinnedCBIndex * skinnedCBSize;
            renderResource->cmdList->SetGraphicsRootConstantBufferView(6, skinnedCBAddress);
        }

        renderResource->cmdList->DrawIndexedInstanced(gObjMeshes->IndexCount, 1, 0, 0, 0);

        meshCounter++;
    }


    return true;
}

bool GameObject::drawShadow() const
{
    const auto gObjRenderItem = renderItem.get();
    const auto objectCB = ServiceProvider::getRenderResource()->getCurrentFrameResource()->ObjectCB->getResource();

    if (!isDrawEnabled || !isShadowEnabled) return false;

    const auto renderResource = ServiceProvider::getRenderResource();

    D3D12_GPU_VIRTUAL_ADDRESS cachedObjCBAddress = 0;

    UINT meshCounter = 0;

    for (const auto& gObjMeshes : gObjRenderItem->getModel()->meshes)
    {
        renderResource->cmdList->IASetVertexBuffers(0, 1, &gObjMeshes->VertexBufferView());
        renderResource->cmdList->IASetIndexBuffer(&gObjMeshes->IndexBufferView());
        renderResource->cmdList->IASetPrimitiveTopology(gObjRenderItem->PrimitiveType);

        D3D12_GPU_VIRTUAL_ADDRESS objCBAddress = objectCB->GetGPUVirtualAddress() + (long long)gObjRenderItem->ObjCBIndex[meshCounter] * objectCBSize;

        /*only if changed*/
        if (cachedObjCBAddress != objCBAddress)
        {
            renderResource->cmdList->SetGraphicsRootConstantBufferView(0, objCBAddress);
            cachedObjCBAddress = objCBAddress;
        }

        if (gObjRenderItem->isSkinned())
        {
            D3D12_GPU_VIRTUAL_ADDRESS skinnedCBAddress = ServiceProvider::getRenderResource()->getCurrentFrameResource()->SkinnedCB->getResource()->GetGPUVirtualAddress() + (long long)gObjRenderItem->SkinnedCBIndex * skinnedCBSize;;
            renderResource->cmdList->SetGraphicsRootConstantBufferView(6, skinnedCBAddress);
        }

        renderResource->cmdList->DrawIndexedInstanced(gObjMeshes->IndexCount, 1, 0, 0, 0);

        meshCounter++;
    }

    return true;
}

void GameObject::drawPickBox() const
{
    const auto gObjRenderItem = renderItem.get();
    const auto objectCB = ServiceProvider::getRenderResource()->getCurrentFrameResource()->ObjectCB->getResource();

    const auto boxMesh = renderItem->getModel()->boundingBoxMesh.get();

    if (boxMesh == nullptr) return;
    if (!isCollisionEnabled) return;

    const auto renderResource = ServiceProvider::getRenderResource();

    renderResource->cmdList->IASetVertexBuffers(0, 1, &boxMesh->VertexBufferView());
    renderResource->cmdList->IASetIndexBuffer(&boxMesh->IndexBufferView());
    renderResource->cmdList->IASetPrimitiveTopology(gObjRenderItem->PrimitiveType);

    D3D12_GPU_VIRTUAL_ADDRESS objCBAddress = objectCB->GetGPUVirtualAddress() + (long long)gObjRenderItem->ObjCBIndex[0] * objectCBSize;

    renderResource->cmdList->SetGraphicsRootConstantBufferView(0, objCBAddress);

    renderResource->cmdList->DrawIndexedInstanced(boxMesh->IndexCount, 1, 0, 0, 0);
}

json GameObject::toJson() const
{
    json jElement;

    jElement["Name"] = Name;
    jElement["Model"] = gameObjectType != GameObjectType::Wall ? renderItem->staticModel->name : "";

    if (renderItem->MaterialOverwrite != nullptr)
    {
        jElement["Material"] = renderItem->MaterialOverwrite->Name;
    }

    switch (renderItem->renderType)
    {
        case RenderType::Default: jElement["RenderType"] = "Default"; break;
        case RenderType::DefaultAlpha: jElement["RenderType"] = "DefaultAlpha"; break;
        case RenderType::DefaultNoNormal: jElement["RenderType"] = "DefaultNoNormal"; break;
        case RenderType::DefaultTransparency: jElement["RenderType"] = "DefaultTransparency"; break;
        case RenderType::NoCullNoNormal: jElement["RenderType"] = "NoCullNoNormal"; break;
        default: LOG(Severity::Warning, "GameObject with impossible RenderType"); break;
    }

    jElement["CollisionEnabled"] = isCollisionEnabled;
    jElement["DrawEnabled"] = isDrawEnabled;
    jElement["ShadowEnabled"] = isShadowEnabled;

    jElement["Position"][0] = getPosition().x;
    jElement["Position"][1] = getPosition().y;
    jElement["Position"][2] = getPosition().z;

    jElement["Scale"][0] = getScale().x;
    jElement["Scale"][1] = getScale().y;
    jElement["Scale"][2] = getScale().z;

    jElement["Rotation"][0] = XMConvertToDegrees(getRotation().x);
    jElement["Rotation"][1] = XMConvertToDegrees(getRotation().y);
    jElement["Rotation"][2] = XMConvertToDegrees(getRotation().z);

    jElement["TexTranslation"][0] = getTextureTranslation().x;
    jElement["TexTranslation"][1] = getTextureTranslation().y;
    jElement["TexTranslation"][2] = getTextureTranslation().z;

    jElement["TexScale"][0] = getTextureScale().x;
    jElement["TexScale"][1] = getTextureScale().y;
    jElement["TexScale"][2] = getTextureScale().z;

    jElement["TexRotation"][0] = XMConvertToDegrees(getTextureRotation().x);
    jElement["TexRotation"][1] = XMConvertToDegrees(getTextureRotation().y);
    jElement["TexRotation"][2] = XMConvertToDegrees(getTextureRotation().z);

    return jElement;
}

void GameObject::makeDynamic(SkinnedModel* sModel, UINT cbIndex)
{
    gameObjectType = GameObjectType::Dynamic;
    renderItem->skinnedModel = sModel;
    setAnimation(nullptr);
}

void GameObject::setAnimation(AnimationClip* aClip, bool keepRelativeTime)
{
    float percentTime = 0.0f;

    if (keepRelativeTime)
    {
        percentTime = renderItem->animationTimer / renderItem->currentClip->getEndTime();
    }

    renderItem->currentClip = aClip;

    if (aClip != nullptr)
    {
        renderItem->animationTimer = percentTime * renderItem->currentClip->getEndTime();
        renderItem->finalTransforms.resize(aClip->boneAnimations.size());
    }
    else
    {
        renderItem->animationTimer = 0.0f;
        renderItem->finalTransforms.resize(96);
    }
    
}

void GameObject::checkInViewFrustum(BoundingFrustum& localCamFrustum)
{

    if (isFrustumCulled)
    {
        currentlyInFrustum = !collider.intersects(localCamFrustum);
    }
    else
    {
        currentlyInFrustum = true;
    }

}

void GameObject::setColliderProperties(GameCollider::GameObjectCollider type, DirectX::XMFLOAT3 center, DirectX::XMFLOAT3 extents)
{
    collider.setColliderType(type);
    collider.setProperties(center, extents);

    updateTransforms();
}

bool GameObject::intersects(const GameObject& obj) const
{
    if (!isCollisionEnabled || !obj.isCollisionEnabled)
    {
        return false;
    }

    return collider.intersects(obj.collider);
}

//bool GameObject::intersectsRough(DirectX::BoundingOrientedBox& box) const
//{
//    if (!isCollisionEnabled)
//    {
//        return false;
//    }
//
//    return roughBoundingBox.Intersects(box);
//}

void GameObject::updateTransforms()
{
    /*update transforms for constant buffer*/
    XMMATRIX rootTransform = XMMatrixIdentity();

    /*for dynamic objects apply scene root transform*/
    if (gameObjectType == GameObjectType::Dynamic)
    {
        rootTransform = XMLoadFloat4x4(&renderItem->skinnedModel->rootTransform);
    }

    XMMATRIX preWorld = XMMatrixScalingFromVector(XMLoadFloat3(&Scale)) *
        XMMatrixRotationRollPitchYawFromVector(XMLoadFloat3(&Rotation)) *
        XMMatrixTranslationFromVector(XMLoadFloat3(&Position));

    XMStoreFloat4x4(&renderItem->World, rootTransform *preWorld);

    XMStoreFloat4x4(&renderItem->TexTransform, XMMatrixScalingFromVector(XMLoadFloat3(&TextureScale)) *
                    XMMatrixRotationRollPitchYawFromVector(XMLoadFloat3(&TextureRotation)) *
                    XMMatrixTranslationFromVector(XMLoadFloat3(&TextureTranslation)));

    /*update collider*/
    collider.update(renderItem->World);



    renderItem->NumFramesDirty = gNumFrameResources;
}