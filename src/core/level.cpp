#include "level.h"

bool Level::load(const std::string& levelFile)
{
    /*open and parse the level file*/
    std::ifstream levelStream (LEVEL_PATH + std::string("/") + levelFile);

    if (!levelStream.is_open())
    {
        LOG(Severity::Critical, "Unable to open the level file " << levelFile << "!");
        return false;
    }

    json levelJson;

    try
    {
        levelJson = json::parse(levelStream);
    }
    catch(...){
        LOG(Severity::Critical, "Error while parsing level file " << levelFile << "! Check JSON validity!")
        return false;
    }

    levelStream.close();

    LOG(Severity::Info, "Parsing of level file " << levelFile << " successful");

    /*parse cameras*/
    auto defaultCamera = std::make_unique<Camera>();
    defaultCamera->setPosition(0.0f, 5.0f, -20.f);

    activeCamera = defaultCamera.get();
    mCameras.push_back(std::move(defaultCamera));

    activeCamera->updateViewMatrix();

    if (!parseCameras(levelJson["Camera"]))
    {

    }

    /*parse terrain*/



    /*parse game objects*/
    if (!parseGameObjects(levelJson["GameObject"]))
    {

    }

    buildFrameResource();

    LOG(Severity::Info, "Loaded level " << levelFile << " successfully.");

    return true;
}

void Level::update(const GameTime& gt)
{
}

void Level::updateBuffers(const GameTime& gt)
{
    updateGameObjectConstantBuffers(gt);
    updateMaterialConstantBuffers(gt);
    updateMainPassConstantBuffers(gt);
}

void Level::draw()
{

    UINT objCBByteSize = d3dUtil::CalcConstantBufferSize(sizeof(ObjectConstants));

    auto objectCB = mCurrentFrameResource->ObjectCB->getResource();
    auto renderResource = ServiceProvider::getRenderResource();

    // For each render item...

    for (const auto& gameObject : mGameObjects)
    {
        const auto gObjRenderItem = gameObject.second->renderItem.get();

        for (const auto& gObjMeshes : gObjRenderItem->Model->meshes)
        {
            renderResource->cmdList->IASetVertexBuffers(0, 1, &gObjMeshes.second->VertexBufferView());
            renderResource->cmdList->IASetIndexBuffer(&gObjMeshes.second->IndexBufferView());
            renderResource->cmdList->IASetPrimitiveTopology(gObjRenderItem->PrimitiveType);

            D3D12_GPU_VIRTUAL_ADDRESS objCBAddress = objectCB->GetGPUVirtualAddress() + (long long)gObjRenderItem->ObjCBIndex * objCBByteSize;

            renderResource->cmdList->SetGraphicsRootConstantBufferView(0, objCBAddress);

            renderResource->cmdList->DrawIndexedInstanced(gObjMeshes.second->IndexCount, 1, 0, 0, 0);
        }

    }

    /* Draw the hitboxes of the GameObjects if enabled */
    if (renderResource->isHitBoxDrawEnabled() == false)return;

    renderResource->cmdList->SetPipelineState(renderResource->mPSOs["hitbox"].Get());

    for (const auto& gameObject : mGameObjects)
    {
        const auto gObjRenderItem = gameObject.second->renderItem.get();

        for (const auto& gObjMeshes : gObjRenderItem->Model->meshes)
        {
            renderResource->cmdList->IASetVertexBuffers(0, 1, &gObjMeshes.second->VertexBufferView());
            renderResource->cmdList->IASetIndexBuffer(&gObjMeshes.second->IndexBufferView());
            renderResource->cmdList->IASetPrimitiveTopology(gObjRenderItem->PrimitiveType);

            D3D12_GPU_VIRTUAL_ADDRESS objCBAddress = objectCB->GetGPUVirtualAddress() + (long long)gObjRenderItem->ObjCBIndex * objCBByteSize;

            renderResource->cmdList->SetGraphicsRootConstantBufferView(0, objCBAddress);

            renderResource->cmdList->DrawIndexedInstanced(gObjMeshes.second->IndexCount, 1, 0, 0, 0);
        }

    }

}

void Level::cycleFrameResource()
{
    mCurrentFrameResourceIndex = (mCurrentFrameResourceIndex + 1) % gNumFrameResources;
    mCurrentFrameResource = mFrameResources[mCurrentFrameResourceIndex].get();
}

int Level::getCurrentFrameResourceIndex()
{
    return mCurrentFrameResourceIndex;
}

FrameResource* Level::getCurrentFrameResource()
{
    return mCurrentFrameResource;
}

bool Level::parseCameras(const json& cameraJson)
{
    return true;
}

bool Level::parseGameObjects(const json& gameObjectJson)
{
    for (auto const& entryJson : gameObjectJson)
    {

        if (!exists(entryJson, "Name") ||
            !exists(entryJson, "Model") ||
            !exists(entryJson, "Material") ||
            !exists(entryJson, "RenderType") ||
            !exists(entryJson, "Position") ||
            !exists(entryJson, "Rotation") ||
            !exists(entryJson, "Scale") ||
            !exists(entryJson, "CollisionEnabled") ||
            !exists(entryJson, "DrawEnabled") ||
            !exists(entryJson, "ShadowEnabled") )
        {
            LOG(Severity::Warning, "Skipping game object due to missing properties!");
            continue;
        }

        auto gameObject = std::make_unique<GameObject>(entryJson, amountGameObjects++);

        mGameObjects[gameObject->name] = std::move(gameObject);

    }


    return true;
}

void Level::buildFrameResource()
{

    for (int i = 0; i < gNumFrameResources; i++)
    {
        mFrameResources.push_back(std::make_unique<FrameResource>(ServiceProvider::getRenderResource()->device,
                                  1,
                                  (UINT)mGameObjects.size(),
                                  0,
                                  (UINT)ServiceProvider::getRenderResource()->mMaterials.size()));
    }

}

void Level::updateGameObjectConstantBuffers(const GameTime& gt)
{

    auto currObjectCB = mCurrentFrameResource->ObjectCB.get();

    for (auto& go : mGameObjects)
    {
        auto e = go.second->renderItem.get();

        // Only update the cbuffer data if the constants have changed.  
        if (e->NumFramesDirty > 0)
        {
            XMMATRIX world = XMLoadFloat4x4(&e->World);
            XMMATRIX texTransform = XMLoadFloat4x4(&e->TexTransform);

            ObjectConstants objConstants;
            XMStoreFloat4x4(&objConstants.World, XMMatrixTranspose(world));
            XMStoreFloat4x4(&objConstants.TexTransform, XMMatrixTranspose(texTransform));
            objConstants.MaterialIndex = e->Mat->MatCBIndex;

            currObjectCB->copyData(e->ObjCBIndex, objConstants);

            // Next FrameResource need to be updated too.
            e->NumFramesDirty--;
        }
    }

}

void Level::updateMainPassConstantBuffers(const GameTime& gt)
{
    /*always update the camera light etc buffer*/
    XMMATRIX view = activeCamera->getView();
    XMMATRIX proj = activeCamera->getProj();

    XMMATRIX viewProj = XMMatrixMultiply(view, proj);
    XMMATRIX invView = XMMatrixInverse(&XMMatrixDeterminant(view), view);
    XMMATRIX invProj = XMMatrixInverse(&XMMatrixDeterminant(proj), proj);
    XMMATRIX invViewProj = XMMatrixInverse(&XMMatrixDeterminant(viewProj), viewProj);

    XMStoreFloat4x4(&mMainPassConstants.View, XMMatrixTranspose(view));
    XMStoreFloat4x4(&mMainPassConstants.InvView, XMMatrixTranspose(invView));
    XMStoreFloat4x4(&mMainPassConstants.Proj, XMMatrixTranspose(proj));
    XMStoreFloat4x4(&mMainPassConstants.InvProj, XMMatrixTranspose(invProj));
    XMStoreFloat4x4(&mMainPassConstants.ViewProj, XMMatrixTranspose(viewProj));
    XMStoreFloat4x4(&mMainPassConstants.InvViewProj, XMMatrixTranspose(invViewProj));
    mMainPassConstants.EyePosW = activeCamera->getPosition3f();



    mMainPassConstants.RenderTargetSize = XMFLOAT2((float)ServiceProvider::getSettings()->displaySettings.ResolutionWidth, (float)ServiceProvider::getSettings()->displaySettings.ResolutionHeight);
    mMainPassConstants.InvRenderTargetSize = XMFLOAT2(1.0f / ServiceProvider::getSettings()->displaySettings.ResolutionWidth, 1.0f / ServiceProvider::getSettings()->displaySettings.ResolutionHeight);
    mMainPassConstants.NearZ = 0.01f;
    mMainPassConstants.FarZ = 1000.0f;
    mMainPassConstants.TotalTime = gt.TotalTime();
    mMainPassConstants.DeltaTime = gt.DeltaTime();
    mMainPassConstants.AmbientLight = { 0.25f, 0.25f, 0.35f, 1.0f };
    mMainPassConstants.Lights[0].Direction = { 0.57735f, -0.57735f, 0.57735f };
    mMainPassConstants.Lights[0].Strength = { 0.8f, 0.8f, 0.8f };
    mMainPassConstants.Lights[1].Direction = { -0.57735f, -0.57735f, 0.57735f };
    mMainPassConstants.Lights[1].Strength = { 0.4f, 0.4f, 0.4f };
    mMainPassConstants.Lights[2].Direction = { 0.0f, -0.707f, -0.707f };
    mMainPassConstants.Lights[2].Strength = { 0.2f, 0.2f, 0.2f };

    auto currPassCB = mCurrentFrameResource->PassCB.get();
    currPassCB->copyData(0, mMainPassConstants);

}

void Level::updateMaterialConstantBuffers(const GameTime& gt)
{

    auto currMaterialBuffer = mCurrentFrameResource->MaterialBuffer.get();
    for (auto& e : ServiceProvider::getRenderResource()->mMaterials)
    {
        // Only update the cbuffer data if the constants have changed.  If the cbuffer
        // data changes, it needs to be updated for each FrameResource.
        Material* mat = e.second.get();
        if (mat->NumFramesDirty > 0)
        {
            XMMATRIX matTransform = XMLoadFloat4x4(&mat->MatTransform);

            MaterialData matData;
            matData.DiffuseAlbedo = mat->DiffuseAlbedo;
            matData.FresnelR0 = mat->FresnelR0;
            matData.Roughness = mat->Roughness;
            XMStoreFloat4x4(&matData.MatTransform, XMMatrixTranspose(matTransform));
            matData.DiffuseMapIndex = mat->DiffuseSrvHeapIndex;
            matData.NormalMapIndex = mat->NormalSrvHeapIndex;

            currMaterialBuffer->copyData(mat->MatCBIndex, matData);

            // Next FrameResource need to be updated too.
            mat->NumFramesDirty--;
        }
    }

}
