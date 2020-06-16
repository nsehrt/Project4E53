#include "gameobject.h"

using namespace DirectX;

GameObject::GameObject(const json& objectJson, int index, int skinnedIndex)
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

    /*simple animation*/
    if (exists(objectJson, "SimpleRotation"))
    {
        isSimpleAnimated = true;
        SimpleRotation = XMFLOAT3{ XMConvertToRadians(objectJson["SimpleRotation"][0]),
                                   XMConvertToRadians(objectJson["SimpleRotation"][1]),
                                   XMConvertToRadians(objectJson["SimpleRotation"][2]) };
    }
    else
    {
        isSimpleAnimated = false;
        SimpleRotation = XMFLOAT3{ 0.0f,0.0f,0.0f };
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
            rItem->staticModel = renderResource->mModels["box"].get();
            TextureScale = Scale;
            gameObjectType = GameObjectType::Wall;
        }
        else
        {
            LOG(Severity::Warning, "GameObject " << name << " specified not loaded model " << objectJson["Model"] << "!");

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
                LOG(Severity::Warning, "GameObject " << name << " specified not loaded material " << objectJson["Material"] << "!");
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
            rItem->shadowType = RenderType::ShadowAlpha;
        }
        else if (objectJson["RenderType"] == "NoCullNoNormal")
        {
            rItem->renderType = RenderType::NoCullNoNormal;
            rItem->shadowType = RenderType::ShadowAlpha;
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
    customFrustumBoundingBoxExtents = { 0.0f,0.0f,0.0f };

    TextureTranslation = XMFLOAT3(0.0f, 0.0f, 0.0f);
    TextureRotation = XMFLOAT3(0.0f, 0.0f, 0.0f);
    TextureScale = XMFLOAT3(1.0f, 1.0f, 1.0f);
    SimpleRotation = XMFLOAT3(0.0f, 0.0f, 0.0f);

    objectCBSize = d3dUtil::CalcConstantBufferSize(sizeof(ObjectConstants));
}

GameObject::GameObject(int index, int skinnedIndex)
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
    tItem->ObjCBIndex.push_back(index); 
    tItem->ObjCBIndex.push_back(index); 
    tItem->ObjCBIndex.push_back(index);
    tItem->MaterialOverwrite = renderResource->mMaterials["default"].get();

    if (skinnedIndex == -1)
    {
        tItem->staticModel = renderResource->mModels["box"].get();
    }
    else
    {
        tItem->SkinnedCBIndex = skinnedIndex;
    }
    

    renderItem = std::move(tItem);

    objectCBSize = d3dUtil::CalcConstantBufferSize(sizeof(ObjectConstants));
}

void GameObject::update(const GameTime& gt)
{
    if (gameObjectType == GameObjectType::Dynamic)
    {
        renderItem->animationTimer += gt.DeltaTime() * animationTimeScale;

        if (renderItem->animationTimer >= renderItem->currentClip->getEndTime())
        {
            renderItem->animationTimer = 0.0f;
        }

        renderItem->skinnedModel->calculateFinalTransforms(renderItem->currentClip, renderItem->animationTimer);
    }


    /*simple animation*/
    if (isSimpleAnimated)
    {
        XMFLOAT3 r = getRotation();
        r.x += SimpleRotation.x * gt.DeltaTime();
        r.y += SimpleRotation.y * gt.DeltaTime();
        r.z += SimpleRotation.z * gt.DeltaTime();

        setRotation(r);
    }

}

bool GameObject::draw()
{
    const auto gObjRenderItem = renderItem.get();
    const auto objectCB = ServiceProvider::getRenderResource()->getCurrentFrameResource()->ObjectCB->getResource();


    if (!isDrawEnabled &&
        !(gameObjectType == GameObjectType::Wall && ServiceProvider::getSettings()->miscSettings.EditModeEnabled))
    {
        return false;
    }

    if (!isInFrustum) return false;

    const auto renderResource = ServiceProvider::getRenderResource();

    D3D12_GPU_VIRTUAL_ADDRESS cachedObjCBAddress = 0;

    UINT meshCounter = 0;

    if (gameObjectType != GameObjectType::Dynamic)
    {
        for (const auto& gObjMeshes : gObjRenderItem->staticModel->meshes)
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

            renderResource->cmdList->DrawIndexedInstanced(gObjMeshes->IndexCount, 1, 0, 0, 0);

            meshCounter++;
        }
    }
    else
    {

        for (const auto& gObjMeshes : gObjRenderItem->skinnedModel->meshes)
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

            D3D12_GPU_VIRTUAL_ADDRESS skinnedCBAddress = ServiceProvider::getRenderResource()->getCurrentFrameResource()->SkinnedCB->getResource()->GetGPUVirtualAddress();
            renderResource->cmdList->SetGraphicsRootConstantBufferView(6, skinnedCBAddress);

            renderResource->cmdList->DrawIndexedInstanced(gObjMeshes->IndexCount, 1, 0, 0, 0);

            meshCounter++;
        }

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

        renderResource->cmdList->DrawIndexedInstanced(gObjMeshes->IndexCount, 1, 0, 0, 0);

        meshCounter++;
    }

    return true;
}

void GameObject::drawRoughHitbox()
{
    const auto gObjRenderItem = renderItem.get();
    const auto objectCB = ServiceProvider::getRenderResource()->getCurrentFrameResource()->ObjectCB->getResource();

    const auto boxMesh = renderItem->isSkinned() ? gObjRenderItem->skinnedModel->boundingBoxMesh.get() : gObjRenderItem->staticModel->boundingBoxMesh.get();

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

json GameObject::toJson()
{
    json jElement;

    jElement["Name"] = name;
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
    jElement["ShadowForced"] = isShadowForced;

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

    if (isSimpleAnimated)
    {
        jElement["SimpleRotation"][0] = XMConvertToDegrees(SimpleRotation.x);
        jElement["SimpleRotation"][1] = XMConvertToDegrees(SimpleRotation.y);
        jElement["SimpleRotation"][2] = XMConvertToDegrees(SimpleRotation.z);
    }

    return jElement;
}

void GameObject::checkInViewFrustum(BoundingFrustum& localCamFrustum)
{

    if (isFrustumCulled)
    {

        XMMATRIX world = XMLoadFloat4x4(&renderItem->World);

        isInFrustum = !(localCamFrustum.Contains(frustumCheckBoundingBox) == DirectX::DISJOINT);

    }
    else
    {
        isInFrustum = true;
    }

}

bool GameObject::intersectsRough(GameObject& obj)
{
    if (!isCollisionEnabled || !obj.isCollisionEnabled)
    {
        return false;
    }

    return roughBoundingBox.Intersects(obj.roughBoundingBox);
}

bool GameObject::intersectsRough(DirectX::BoundingOrientedBox& box)
{
    if (!isCollisionEnabled)
    {
        return false;
    }

    return roughBoundingBox.Intersects(box);
}

bool GameObject::intersectsShadowBounds(DirectX::BoundingSphere& sphere)
{
    return frustumCheckBoundingBox.Intersects(sphere);
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

    /*update rough hitbox*/
    auto model = renderItem->getModel();

    if (model)
    {
        model->boundingBox.Transform(roughBoundingBox, XMLoadFloat4x4(&renderItem->World));
        model->frustumBoundingBox.Transform(frustumCheckBoundingBox, XMLoadFloat4x4(&renderItem->World));
    }


    if (useCustomFrustumBoundingBoxExtents)
    {
        frustumCheckBoundingBox.Extents = customFrustumBoundingBoxExtents;
    }

    /*update for precise hitbox needed*/

    renderItem->NumFramesDirty = gNumFrameResources;
}