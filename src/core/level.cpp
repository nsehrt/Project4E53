#include "level.h"

bool Level::load(const std::string& levelFile)
{
    auto startTime = std::chrono::system_clock::now();

    LOG(Severity::Info, "Loading level " << levelFile << "...");

    /*open and parse the level file*/
    std::ifstream levelStream (LEVEL_PATH + std::string("/") + levelFile);

    if (!levelStream.is_open())
    {
        LOG(Severity::Critical, "Unable to open the level file!");
        return false;
    }

    json levelJson;

    try
    {
        levelJson = json::parse(levelStream);
    }
    catch(nlohmann::detail::parse_error){
        LOG(Severity::Critical, "Error while parsing level file! Check JSON validity!")
        return false;
    }
    catch (...)
    {
        LOG(Severity::Critical, "Unknown error with level file!")
        return false;
    }

    levelStream.close();

    /*parse sky*/
    if (!parseSky(levelJson["Sky"]))
    {
        LOG(Severity::Critical, "Failed to load sky sphere properties!");
        return false;
    }

    /*parse cameras*/
    auto defaultCamera = std::make_shared<Camera>();
    defaultCamera->setPosition(0.0f, 5.0f, -20.f);

    ServiceProvider::setActiveCamera(defaultCamera);

    mCameras.push_back(std::move(defaultCamera));

    ServiceProvider::getActiveCamera()->updateViewMatrix();

    if (!parseCameras(levelJson["Camera"]))
    {
        LOG(Severity::Critical, "Failed to load cameras!");
        return false;
    }

    /*parse terrain*/



    /*parse game objects*/
    if (!parseGameObjects(levelJson["GameObject"]))
    {
        LOG(Severity::Critical, "Failed to load Game Objects!");
        return false;
    }

    buildFrameResource();

    auto endTime = std::chrono::system_clock::now();
    std::chrono::duration<double> elapsedTime = endTime - startTime;

    LOG(Severity::Info, "Loaded level " << levelFile << " successfully. (" << elapsedTime.count() << " seconds, " << amountGameObjects << " GameObjects)");

    return true;
}

void Level::update(const GameTime& gt)
{

    for (auto& gameObj : mGameObjects)
    {
        gameObj.second->update(gt);
    }

    /*Collision test*/

    for (const auto& gameObj : mGameObjects)
    {
        if (gameObj.second->gameObjectType != GameObjectType::Static) continue;

        if (gameObj.second->intersects(ServiceProvider::getActiveCamera()->hitbox))
        {
            LOG(Severity::Info, "Camera collided with " << gameObj.second->name);
        }

    }

    if (mGameObjects["box1"]->intersects(*mGameObjects["box2"].get()))
    {
        //LOG(Severity::Info, "Box collision");
    }

    //for (auto& gameObj : mGameObjects)
    //{
    //    if (gameObj.second->gameObjectType != GameObjectType::Static) continue;

    //    auto h = gameObj.second->hitBox;

    //    for (auto& gameObj2 : mGameObjects)
    //    {
    //        if (gameObj == gameObj2) continue;
    //        if (gameObj2.second->gameObjectType != GameObjectType::Static) continue;

    //        if (h.Intersects(gameObj2.second->hitBox))
    //        {
    //            LOG(Severity::Info, "Collision between " << gameObj.second->name << " and " << gameObj2.second->name);
    //        }
    //    }
    //}

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

    /* TODO sort draw order of game objects*/


    // draw the gameobjects
    UINT objectsDrawn = 0;

    for (const auto& gameObject : mGameObjects)
    {

        auto g = gameObject.second.get();

        if (g->renderItem->renderType == RenderType::Default)
        {
            objectsDrawn += gameObject.second->draw(objectCB);
        }
        
    }



    renderResource->cmdList->SetPipelineState(renderResource->mPSOs["defaultAlpha"].Get());

    for (const auto& gameObject : mGameObjects)
    {
        auto g = gameObject.second.get();

        if (g->renderItem->renderType == RenderType::DefaultAlpha)
        {
            objectsDrawn += gameObject.second->draw(objectCB);
        }
    }



    LOG(Severity::Info, objectsDrawn << " objects drawn.");

    /*draw sky sphere*/
    renderResource->cmdList->SetPipelineState(renderResource->mPSOs["sky"].Get());

    for (const auto& gameObject : mGameObjects)
    {
        auto g = gameObject.second.get();

        if (g->renderItem->renderType == RenderType::Sky)
        {
            gameObject.second->draw(objectCB);
        }
        
    }

    /* Draw the hitboxes of the GameObjects if enabled */
    if (renderResource->isHitBoxDrawEnabled())
    {

        renderResource->cmdList->SetPipelineState(renderResource->mPSOs["hitbox"].Get());

        for (const auto& gameObject : mGameObjects)
        {
            auto g = gameObject.second.get();

            if (g->renderItem->renderType == RenderType::Default ||
                g->renderItem->renderType == RenderType::DefaultAlpha
                )
            {
                gameObject.second->drawHitbox(objectCB);
            }
            
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

bool Level::parseSky(const json& skyJson)
{
    if (!exists(skyJson, "Material"))
    {
        return false;
    }

    auto renderResource = ServiceProvider::getRenderResource();
    auto gameObject = std::make_unique<GameObject>();

    auto rItem = std::make_unique<RenderItem>();

    XMStoreFloat4x4(&rItem->World, XMMatrixScaling(5000.0f, 5000.0f, 5000.f));
    rItem->TexTransform = MathHelper::identity4x4();
    rItem->ObjCBIndex = amountGameObjects++;
    rItem->Mat = renderResource->mMaterials[skyJson["Material"]].get();
    rItem->Model = renderResource->mModels["sphere"].get();
    rItem->renderType = RenderType::Sky;

    gameObject->name = "SkySphere";
    gameObject->renderItem = std::move(rItem);
    gameObject->gameObjectType = GameObjectType::Sky;
    gameObject->isFrustumCulled = false;

    mGameObjects[gameObject->name] = std::move(gameObject);

    return true;
}

bool Level::parseCameras(const json& cameraJson)
{
    return true;
}

bool Level::parseGameObjects(const json& gameObjectJson)
{
    for (auto const& entryJson : gameObjectJson)
    {
        if (!exists(entryJson, "Name"))
        {
            LOG(Severity::Warning, "GameObject is missing the name property!");
            continue;
        }

        if (!exists(entryJson, "Model") ||
            !exists(entryJson, "Material") ||
            !exists(entryJson, "RenderType")
            )
        {
            LOG(Severity::Warning, "Skipping GameObject " << entryJson["Name"] << "due to missing properties!");
            continue;
        }

        if (mGameObjects.find(entryJson["Name"]) != mGameObjects.end())
        {
            LOG(Severity::Warning, "GameObject " << entryJson["Name"] << " already exists!");
            return false;
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
    XMMATRIX view = ServiceProvider::getActiveCamera()->getView();
    XMMATRIX proj = ServiceProvider::getActiveCamera()->getProj();

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
    mMainPassConstants.EyePosW = ServiceProvider::getActiveCamera()->getPosition3f();



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
