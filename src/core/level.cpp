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

    /*TODO Collision test*/

    for (const auto& gameObj : mGameObjects)
    {
        if (gameObj.second->gameObjectType != GameObjectType::Static) continue;

        if (gameObj.second->intersects(ServiceProvider::getActiveCamera()->hitbox))
        {
            //LOG(Severity::Info, "Camera collided with " << gameObj.second->name);
            ServiceProvider::getActiveCamera()->setPosition(ServiceProvider::getActiveCamera()->mPreviousPosition);
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


    ServiceProvider::getShadowMap()->mLightRotationAngle += 0.1f * gt.DeltaTime();

    XMMATRIX R = XMMatrixRotationY(ServiceProvider::getShadowMap()->mLightRotationAngle);
    for (int i = 0; i < 3; ++i)
    {
        XMVECTOR lightDir = XMLoadFloat3(&ServiceProvider::getShadowMap()->mBaseLightDirections[i]);
        lightDir = XMVector3TransformNormal(lightDir, R);
        XMStoreFloat3(&ServiceProvider::getShadowMap()->mRotatedLightDirections[i], lightDir);
    }

}

void Level::draw(int drawMode)
{

    UINT objCBByteSize = d3dUtil::CalcConstantBufferSize(sizeof(ObjectConstants));

    auto renderResource = ServiceProvider::getRenderResource();
    auto objectCB = renderResource->getCurrentFrameResource()->ObjectCB->getResource();
    

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

    renderResource->cmdList->SetPipelineState(renderResource->mPSOs["defaultNoNormal"].Get());

    for (const auto& gameObject : mGameObjects)
    {
        auto g = gameObject.second.get();

        if (g->renderItem->renderType == RenderType::DefaultNoNormal)
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


    ServiceProvider::getDebugInfo()->DrawnGameObjects = objectsDrawn;

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

            if (g->renderItem->renderType != RenderType::Sky)
            {
                gameObject.second->drawHitbox(objectCB);
            }
            
        }

    }

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
