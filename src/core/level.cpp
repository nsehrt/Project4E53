#include "level.h"

bool Level::load(const std::string& levelFile)
{
    auto startTime = std::chrono::system_clock::now();

    LOG(Severity::Info, "Loading level " << levelFile << "...");

    /*open and parse the level file*/
    std::ifstream levelStream(LEVEL_PATH + std::string("/") + levelFile);

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
    catch (nlohmann::detail::parse_error)
    {
        LOG(Severity::Critical, "Error while parsing level file! Check JSON validity!")
            return false;
    }
    catch (...)
    {
        LOG(Severity::Critical, "Unknown error with level file!")
            return false;
    }

    levelStream.close();

    /*parse terrain*/
    if (!parseTerrain(levelJson["Terrain"]))
    {
        LOG(Severity::Critical, "Failed to load terrain!");
        return false;
    }

    /*parse sky*/
    if (!parseSky(levelJson["Sky"]))
    {
        LOG(Severity::Critical, "Failed to load sky sphere properties!");
        return false;
    }

    /*parse water*/
    if (!parseWater(levelJson["Water"]))
    {
        LOG(Severity::Critical, "Failed to load water!");
        return false;
    }

    /*parse lights*/
    if (!parseLights(levelJson["Light"]))
    {
        LOG(Severity::Critical, "Failed to load lights!");
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

    /*parse game objects*/
    if (!parseGameObjects(levelJson["GameObject"]))
    {
        LOG(Severity::Critical, "Failed to load Game Objects!");
        return false;
    }

    calculateRenderOrder();

    auto endTime = std::chrono::system_clock::now();
    std::chrono::duration<double> elapsedTime = endTime - startTime;

    LOG(Severity::Info, "Loaded level " << levelFile << " successfully. (" << elapsedTime.count() << " seconds, " << amountGameObjects << " GameObjects)");

    loadedLevel = LEVEL_PATH + std::string("/") + levelFile;

    return true;
}

void Level::update(const GameTime& gt)
{
    auto renderResource = ServiceProvider::getRenderResource();
    auto aCamera = ServiceProvider::getActiveCamera();

    /*udpdate all game objects*/
    for (auto& gameObj : mGameObjects)
    {
        gameObj.second->update(gt);
    }

    /*update order of light objects*/

    auto cameraPos = aCamera->getPosition();

    /*TODO mit Spielerposition statt Kameraposition*/
    std::sort(mLightObjects.begin() + AMOUNT_DIRECTIONAL,
              mLightObjects.end() - AMOUNT_SPOT,
              [&](const std::unique_ptr<LightObject>& a, const std::unique_ptr<LightObject>& b) -> bool
              {
                  XMVECTOR lengthA = XMVector3LengthSq(XMLoadFloat3(&a->getPosition()) - cameraPos);
                  XMVECTOR lengthB = XMVector3LengthSq(XMLoadFloat3(&b->getPosition()) - cameraPos);

                  XMFLOAT3 result;
                  XMStoreFloat3(&result, XMVectorLess(lengthA, lengthB));

                  return result.x;
              });

    for (UINT i = 3; i < (MAX_LIGHTS - 1); i++)
    {
        if (i < mLightObjects.size() - 1)
            mCurrentLightObjects[i] = mLightObjects[i].get();
    }

    /*water*/
    updateWater(gt);

    /*TODO Collision test*/
    if(!ServiceProvider::getSettings()->miscSettings.EditModeEnabled)
    for (const auto& gameObj : mGameObjects)
    {
        if (gameObj.second->gameObjectType != GameObjectType::Static &&
            gameObj.second->gameObjectType != GameObjectType::Wall) continue;

        if (gameObj.second->intersects(ServiceProvider::getActiveCamera()->hitbox))
        {
            //LOG(Severity::Info, "Camera collided with " << gameObj.second->name);
            ServiceProvider::getActiveCamera()->setPosition(ServiceProvider::getActiveCamera()->mPreviousPosition);
        }
    }

    //if (mGameObjects["box1"]->intersects(*mGameObjects["box2"].get()))
    //{
        //LOG(Severity::Info, "Box collision");
    //}

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

void Level::updateWater(const GameTime& gt)
{

    auto waterMat = ServiceProvider::getRenderResource()->mMaterials["water"].get();

    float& tu = waterMat->MatTransform(3, 0);
    float& tv = waterMat->MatTransform(3, 1);

    tu += 0.085f * gt.DeltaTime();
    tv += 0.02f * gt.DeltaTime();

    /*reset if > 1*/
    if (tu >= 1.0f)
        tu -= 1.0f;

    if (tv >= 1.0f)
        tv -= 1.0f;

    waterMat->MatTransform(3, 0) = tu;
    waterMat->MatTransform(3, 1) = tv;

    // Material has changed, so need to update cbuffer.
    waterMat->NumFramesDirty = gNumFrameResources;

}

void Level::drawWater()
{
    auto renderResource = ServiceProvider::getRenderResource();

    renderResource->setPSO(RenderType::Water);

    for (auto const& i : mGameObjects)
    {
        if (i.second->renderItem->renderType == RenderType::Water)
        {
            i.second->draw();
            break;
        }
    }
}

void Level::drawTerrain()
{
    for (auto const& i : mGameObjects)
    {
        if (i.second->renderItem->renderType == RenderType::Terrain)
        {
            i.second->draw();
            break;
        }
    }
}

void Level::draw()
{
    UINT objCBByteSize = d3dUtil::CalcConstantBufferSize(sizeof(ObjectConstants));

    auto renderResource = ServiceProvider::getRenderResource();

    /* Order of render items*/
    for (auto& v : renderOrder)
    {
        v.clear();
    }

    for (const auto& gameObject : mGameObjects)
    {
        renderOrder[(int)gameObject.second->renderItem->renderType].push_back(&(*gameObject.second));
    }

    /*sort the transparent objects by distance from camera*/
    auto aCamera = ServiceProvider::getActiveCamera();
    XMVECTOR cameraPos = aCamera->getPosition();

    std::sort(renderOrder[(int)RenderType::DefaultTransparency].begin(),
              renderOrder[(int)RenderType::DefaultTransparency].end(),
              [&](const GameObject* a, const GameObject* b) -> bool
              {
                  XMVECTOR lengthA = XMVector3LengthSq(XMLoadFloat3(&a->getPosition()) - cameraPos);
                  XMVECTOR lengthB = XMVector3LengthSq(XMLoadFloat3(&b->getPosition()) - cameraPos);

                  XMFLOAT3 result;
                  XMStoreFloat3(&result, XMVectorGreater(lengthA, lengthB));

                  return result.x;
              });

    // draw the gameobjects
    UINT objectsDrawn = 0;

    for (UINT i = 0; i < renderOrder.size(); i++)
    {
        if (renderOrder[i].empty())continue;
        if ((i == (UINT)RenderType::Debug && !ServiceProvider::getSettings()->miscSettings.DebugEnabled)
            || (i == (UINT)RenderType::Debug && !ServiceProvider::getSettings()->miscSettings.DebugQuadEnabled)
            || i == (UINT)RenderType::Terrain || i == (UINT)RenderType::Water
            ) continue;

        /*set PSO*/

        renderResource->setPSO(RenderType(i));

        for (const auto& gameObject : renderOrder[i])
        {
            objectsDrawn += gameObject->draw();
        }
    }

    ServiceProvider::getDebugInfo()->DrawnGameObjects = objectsDrawn;

    /* Draw the hitboxes of the GameObjects if enabled */
    if (renderResource->isHitBoxDrawEnabled())
    {
        renderResource->setPSO(RenderType::Hitbox);

        for (const auto& gameObject : mGameObjects)
        {
            auto g = gameObject.second.get();

            if (g->renderItem->renderType != RenderType::Sky)
            {
                gameObject.second->drawHitbox();
            }
        }
    }
}

bool Level::save()
{
    auto startTime = std::chrono::system_clock::now();

    /*save level file*/
    json saveFile;

    /*sky*/
    saveFile["Sky"]["Material"] = skyMaterial;
    saveFile["Sky"]["DefaultCubeMap"] = defaultCubeMapStr;

    /*terrain*/
    saveFile["Terrain"]["HeightMap"] = mTerrain->terrainHeightMapFileStem;
    saveFile["Terrain"]["BlendMap"] = mTerrain->terrainBlendMapFileStem;
    saveFile["Terrain"]["HeightScale"] = mTerrain->heightScale;
    for (int i = 0; i < 4; i++)
    {
        saveFile["Terrain"]["BlendTextures"].push_back(mTerrain->textureStrings[i]);
    }

    /*light*/
    saveFile["Light"]["AmbientLight"][0] = AmbientLight.x;
    saveFile["Light"]["AmbientLight"][1] = AmbientLight.y;
    saveFile["Light"]["AmbientLight"][2] = AmbientLight.z;

    for (const auto& e : mLightObjects)
    {
        if (e->getLightType() == LightType::Directional)
        {
            json jElement;

            jElement["Name"] = e->name;
            jElement["Strength"][0] = e->getStrength().x;
            jElement["Strength"][1] = e->getStrength().y;
            jElement["Strength"][2] = e->getStrength().z;
            jElement["Direction"][0] = e->getDirection().x;
            jElement["Direction"][1] = e->getDirection().y;
            jElement["Direction"][2] = e->getDirection().z;

            saveFile["Light"]["Directional"].push_back(jElement);
        }
        else if (e->getLightType() == LightType::Point)
        {
            json jElement;

            jElement["Name"] = e->name;
            jElement["Strength"][0] = e->getStrength().x;
            jElement["Strength"][1] = e->getStrength().y;
            jElement["Strength"][2] = e->getStrength().z;
            jElement["Position"][0] = e->getPosition().x;
            jElement["Position"][1] = e->getPosition().y;
            jElement["Position"][2] = e->getPosition().z;
            jElement["FallOffStart"] = e->getFallOffStart();
            jElement["FallOffEnd"] = e->getFallOffEnd();

            saveFile["Light"]["Point"].push_back(jElement);
        }
        else
        {
            json jElement;

            jElement["Name"] = e->name;
            jElement["Strength"][0] = e->getStrength().x;
            jElement["Strength"][1] = e->getStrength().y;
            jElement["Strength"][2] = e->getStrength().z;
            jElement["Position"][0] = e->getPosition().x;
            jElement["Position"][1] = e->getPosition().y;
            jElement["Position"][2] = e->getPosition().z;
            jElement["Direction"][0] = e->getDirection().x;
            jElement["Direction"][1] = e->getDirection().y;
            jElement["Direction"][2] = e->getDirection().z;
            jElement["FallOffStart"] = e->getFallOffStart();
            jElement["FallOffEnd"] = e->getFallOffEnd();
            jElement["SpotPower"] = e->getSpotPower();

            saveFile["Light"]["Spot"].push_back(jElement);
        }
    }

    /*gameobject*/

    for (const auto& e : mGameObjects)
    {
        if (e.second->renderItem->renderType == RenderType::Composite ||
            e.second->renderItem->renderType == RenderType::Terrain ||
            e.second->renderItem->renderType == RenderType::TerrainNoShadow ||
            e.second->renderItem->renderType == RenderType::TerrainWireFrame ||
            e.second->renderItem->renderType == RenderType::Sobel ||
            e.second->renderItem->renderType == RenderType::Sky ||
            e.second->renderItem->renderType == RenderType::Debug ||
            e.second->renderItem->renderType == RenderType::Hitbox ||
            e.second->renderItem->renderType == RenderType::ShadowAlpha ||
            e.second->renderItem->renderType == RenderType::ShadowDefault)
            continue;

       saveFile["GameObject"].push_back(e.second->toJson());
    }

    std::ofstream file(loadedLevel);

    if (!file.is_open())
    {
        LOG(Severity::Error, "Can not write to " << loadedLevel << "!");
        return false;
    }

    file << saveFile.dump(4);
    file.close();

    /*save terrain*/
    if (!mTerrain->save())
    {
        return false;
    }

    LOG(Severity::Info, "Successfully wrote level data to file " << loadedLevel << ".");

    auto endTime = std::chrono::system_clock::now();
    std::chrono::duration<double> elapsedTime = endTime - startTime;

    LOG(Severity::Info, "Level successfully saved. (" << elapsedTime.count() << " seconds)");

    return true;
}

bool Level::createNew(const std::string& levelFile)
{
    json newLevel;

    newLevel["Sky"]["DefaultCubeMap"] = "grasscube1024.dds";
    newLevel["Sky"]["Material"] = "sky";

    newLevel["Terrain"]["BlendMap"] = levelFile + ".bld";
    newLevel["Terrain"]["HeightMap"] = levelFile + ".raw";
    newLevel["Terrain"]["HeightScale"] = 200.0f;
    newLevel["Terrain"]["BlendTextures"][0] = "ForestFloorGreen.dds";
    newLevel["Terrain"]["BlendTextures"][1] = "DirtGround.dds";
    newLevel["Terrain"]["BlendTextures"][2] = "FlowerField.dds";
    newLevel["Terrain"]["BlendTextures"][3] = "SimpleSand.dds";

    json light1;

    light1["Name"] = "D1";
    light1["Direction"][0] = -0.57f;
    light1["Direction"][1] = -0.57f;
    light1["Direction"][2] = 0.57f;
    light1["Strength"][0] = 0.8f;
    light1["Strength"][1] = 0.8f;
    light1["Strength"][2] = 0.8f;

    newLevel["Light"]["Directional"].push_back(light1);

    json light2;

    light2["Name"] = "D2";
    light2["Direction"][0] = 0.57f;
    light2["Direction"][1] = -0.57f;
    light2["Direction"][2] = 0.57f;
    light2["Strength"][0] = 0.4f;
    light2["Strength"][1] = 0.4f;
    light2["Strength"][2] = 0.4f;

    newLevel["Light"]["Directional"].push_back(light2);

    json light3;

    light3["Name"] = "D3";
    light3["Direction"][0] = 0.0f;
    light3["Direction"][1] = -0.7f;
    light3["Direction"][2] = -0.7f;
    light3["Strength"][0] = 0.2f;
    light3["Strength"][1] = 0.2f;
    light3["Strength"][2] = 0.2f;

    newLevel["Light"]["Directional"].push_back(light3);

    json light4;

    light4["Name"] = "P1";
    light4["Position"][0] = 0.0f;
    light4["Position"][1] = 2.0f;
    light4["Position"][2] = 0.0f;
    light4["Strength"][0] = 1.0f;
    light4["Strength"][1] = 1.0f;
    light4["Strength"][2] = 1.0f;
    light4["FallOffStart"] = 4.0f;
    light4["FallOffEnd"] = 5.0f;

    newLevel["Light"]["Point"].push_back(light4);

    json light5;

    light5["Name"] = "P2";
    light5["Position"][0] = -3.0f;
    light5["Position"][1] = 3.0f;
    light5["Position"][2] = 0.0f;
    light5["Strength"][0] = 1.0f;
    light5["Strength"][1] = 0.0f;
    light5["Strength"][2] = 0.25f;
    light5["FallOffStart"] = 10.0f;
    light5["FallOffEnd"] = 12.0f;

    newLevel["Light"]["Point"].push_back(light5);

    newLevel["Light"]["AmbientLight"] = { 0.3f,0.3f,0.3f };

    json gameObj;

    gameObj["Name"] = "box";
    gameObj["Model"] = "box";
    gameObj["RenderType"] = "Default";
    gameObj["Material"] = "default";

    newLevel["GameObject"].push_back(gameObj);

    std::ofstream out (LEVEL_PATH + std::string("/") + levelFile + ".level");

    if (!out.is_open())
    {
        return false;
    }

    out << newLevel.dump(4);
    out.close();

    return true;
}

bool Level::levelExists(const std::string& levelFile)
{
    return std::filesystem::exists(LEVEL_PATH + std::string("/") + levelFile);
}

void Level::drawShadow()
{
    UINT objCBByteSize = d3dUtil::CalcConstantBufferSize(sizeof(ObjectConstants));

    auto renderResource = ServiceProvider::getRenderResource();
    auto objectCB = renderResource->getCurrentFrameResource()->ObjectCB->getResource();

    UINT objectsDrawn = 0;

    /*order shadow render order*/
    for (auto& v : shadowRenderOrder)
    {
        v.clear();
    }

    for (const auto& gameObject : mGameObjects)
    {
        if (gameObject.second->renderItem->renderType == RenderType::Sky ||
            gameObject.second->renderItem->renderType == RenderType::Debug ||
            gameObject.second->renderItem->renderType == RenderType::Terrain) continue;

        shadowRenderOrder[(long long)gameObject.second->renderItem->shadowType - ((int)RenderType::COUNT - 2)].push_back(&(*gameObject.second));
    }

    /*draw shadows*/
    for (UINT i = 0; i < shadowRenderOrder.size(); i++)
    {
        renderResource->setPSO(RenderType((int)RenderType::ShadowDefault + i));

        for (const auto& gameObject : shadowRenderOrder[i])
        {
            if (gameObject->intersectsShadowBounds(renderResource->getShadowMap()->maxShadowDraw))
            {
                objectsDrawn += gameObject->drawShadow();
            }
        }
    }

    ServiceProvider::getDebugInfo()->DrawnShadowObjects = objectsDrawn;
}

void Level::calculateRenderOrder()
{
    /*determine size of render orders*/
    std::vector<int> renderOrderSize((int)RenderType::COUNT);
    renderOrder.clear();
    shadowRenderOrder.clear();

    for (const auto& gameOject : mGameObjects)
    {
        renderOrderSize[(int)gameOject.second->renderItem->renderType]++;
        renderOrderSize[(int)gameOject.second->renderItem->shadowType]++;
    }

    for (int i = 0; i < renderOrderSize.size(); i++)
    {
        if (i < (int)RenderType::COUNT - 2)
        {
            renderOrder.push_back(std::vector<GameObject*>(renderOrderSize[i]));
        }
        else
        {
            shadowRenderOrder.push_back(std::vector<GameObject*>(renderOrderSize[i]));
        }
    }
}

bool Level::parseSky(const json& skyJson)
{
    if (!exists(skyJson, "Material") || !exists(skyJson, "DefaultCubeMap"))
    {
        LOG(Severity::Error, "Missing sky properties!");
        return false;
    }

    auto renderResource = ServiceProvider::getRenderResource();
    auto gameObject = std::make_unique<GameObject>();

    auto rItem = std::make_unique<RenderItem>();

    XMStoreFloat4x4(&rItem->World, XMMatrixScaling(5000.0f, 5000.0f, 5000.f));
    rItem->TexTransform = MathHelper::identity4x4();
    rItem->ObjCBIndex.push_back(amountObjectCBs++);
    rItem->MaterialOverwrite = renderResource->mMaterials[skyJson["Material"]].get();
    rItem->Model = renderResource->mModels["sphere"].get();
    rItem->renderType = RenderType::Sky;

    gameObject->name = "SKY_SPHERE";
    gameObject->renderItem = std::move(rItem);
    gameObject->gameObjectType = GameObjectType::Sky;
    gameObject->isFrustumCulled = false;

    mGameObjects[gameObject->name] = std::move(gameObject);

    /*default cube map*/
    CD3DX12_GPU_DESCRIPTOR_HANDLE tempHandle(renderResource->mSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
    tempHandle.Offset(renderResource->mTextures[skyJson["DefaultCubeMap"]]->index, renderResource->mCbvSrvUavDescriptorSize);
    defaultCubeMapStr = skyJson["DefaultCubeMap"];
    skyMaterial = skyJson["Material"];

    defaultCubeMapHandle = tempHandle;

    return true;
}

bool Level::parseLights(const json& lightJson)
{
    if (!exists(lightJson, "AmbientLight"))
    {
        LOG(Severity::Warning, "Level misses AmbientLight property!");
    }
    else
    {
        AmbientLight.x = lightJson["AmbientLight"][0];
        AmbientLight.y = lightJson["AmbientLight"][1];
        AmbientLight.z = lightJson["AmbientLight"][2];
    }

    /*directional light*/

    UINT lightCounter = 0;

    if (exists(lightJson, "Directional"))
    {
        for (auto const& entryJson : lightJson["Directional"])
        {
            if (!exists(entryJson, "Name"))
            {
                LOG(Severity::Warning, "LightObject is missing the name property!");
                continue;
            }

            if (lightCounter == AMOUNT_DIRECTIONAL) break;

            auto lightObj = std::make_unique<LightObject>(LightType::Directional, entryJson);

            mLightObjects.push_back(std::move(lightObj));
        }
    }

    /*point light*/
    lightCounter = 0;

    if (exists(lightJson, "Point"))
    {
        for (auto const& entryJson : lightJson["Point"])
        {
            if (!exists(entryJson, "Name"))
            {
                LOG(Severity::Warning, "LightObject is missing the name property!");
                continue;
            }

            auto lightObj = std::make_unique<LightObject>(LightType::Point, entryJson);

            mLightObjects.push_back(std::move(lightObj));
        }
    }

    /*spot light*/
    lightCounter = 0;

    if (exists(lightJson, "Spot"))
    {
        for (auto const& entryJson : lightJson["Spot"])
        {
            if (!exists(entryJson, "Name"))
            {
                LOG(Severity::Warning, "LightObject is missing the name property!");
                continue;
            }

            auto lightObj = std::make_unique<LightObject>(LightType::Spot, entryJson);
            mLightObjects.push_back(std::move(lightObj));

            break;
        }
    }

    /*set immutable lights*/

    /*directional*/
    mCurrentLightObjects[0] = mLightObjects[0].get();
    mCurrentLightObjects[1] = mLightObjects[1].get();
    mCurrentLightObjects[2] = mLightObjects[2].get();

    /*spot*/
    mCurrentLightObjects[7] = mLightObjects[mLightObjects.size() - 1].get();

    return true;
}

bool Level::parseCameras(const json& cameraJson) /*TODO*/
{
    return true;
}

bool Level::parseGameObjects(const json& gameObjectJson)
{
    for (const auto& entryJson : gameObjectJson)
    {
        if (!exists(entryJson, "Name"))
        {
            LOG(Severity::Warning, "GameObject is missing the name property!");
            continue;
        }

        if (!exists(entryJson, "Model")
            )
        {
            LOG(Severity::Warning, "Skipping GameObject " << entryJson["Name"] << "due to missing properties!");
            continue;
        }

        if (mGameObjects.find(entryJson["Name"]) != mGameObjects.end())
        {
            LOG(Severity::Warning, "GameObject " << entryJson["Name"] << " already exists!");
            continue;
        }

        auto gameObject = std::make_unique<GameObject>(entryJson, amountObjectCBs);
        amountObjectCBs += 4;// (int)gameObject->renderItem->Model->meshes.size();

        mGameObjects[gameObject->name] = std::move(gameObject);
    }

    auto debugObject = std::make_unique<GameObject>(amountObjectCBs);

    debugObject->name = "debug";
    debugObject->isFrustumCulled = false;
    debugObject->isShadowEnabled = false;
    debugObject->renderItem->renderType = RenderType::Debug;
    debugObject->gameObjectType = GameObjectType::Debug;
    debugObject->renderItem->Model = ServiceProvider::getRenderResource()->mModels["quad"].get();

    mGameObjects["debugQuad"] = std::move(debugObject);

    return true;
}

bool Level::parseTerrain(const json& terrainJson)
{
    if (!exists(terrainJson, "HeightMap") || !exists(terrainJson, "BlendTextures") ||
        !exists(terrainJson, "BlendMap") || !exists(terrainJson, "HeightScale"))
    {
        LOG(Severity::Error, "Missing terrain properties!");
        return false;
    }

    mTerrain = std::make_unique<Terrain>(terrainJson);

    /*create terrain gameobject*/
    auto terrainObject = std::make_unique<GameObject>(amountObjectCBs++);

    terrainObject->name = "TERRAIN";
    terrainObject->isFrustumCulled = false;
    terrainObject->isShadowEnabled = false;
    terrainObject->isDrawEnabled = true;
    terrainObject->isCollisionEnabled = false;
    terrainObject->renderItem->renderType = RenderType::Terrain;
    terrainObject->gameObjectType = GameObjectType::Terrain;
    terrainObject->renderItem->Model = mTerrain->terrainModel.get();
    terrainObject->renderItem->MaterialOverwrite = ServiceProvider::getRenderResource()->mMaterials["terrain"].get();
    XMStoreFloat4x4(&terrainObject->renderItem->TexTransform, XMMatrixScaling((float)mTerrain->terrainSlices / 8,
                    (float)mTerrain->terrainSlices / 8, (float)mTerrain->terrainSlices / 8));

    mGameObjects["TERRAIN"] = std::move(terrainObject);

    return true;
}

bool Level::parseWater(const json& waterJson)
{
    for (const auto& entry : waterJson)
    {
        /*check validity*/
        if (!exists(entry, "Name"))
        {
            LOG(Severity::Error, "Water is missing name!");
            return false;
        }
        if (!exists(entry, "Material"))
        {
            LOG(Severity::Error, "Water is missing material!");
            return false;
        }
        if (!exists(entry, "Position"))
        {
            LOG(Severity::Error, "Water is missing position!");
        }

        if (mGameObjects.find(entry["Name"]) != mGameObjects.end())
        {
            LOG(Severity::Warning, "Water " << entry["Name"] << " already exists!");
            continue;
        }

        auto waterObject = std::make_unique<GameObject>(amountObjectCBs++);
        XMFLOAT3 pos;

        waterObject->name = entry["Name"];

        pos.x = entry["Position"][0];
        pos.y = entry["Position"][1];
        pos.z = entry["Position"][2];
        waterObject->setPosition(pos);

        waterObject->isShadowEnabled = false;
        waterObject->isCollisionEnabled = false;
        waterObject->isShadowForced = false;
        waterObject->isFrustumCulled = true; /*TODO*/
        waterObject->gameObjectType = GameObjectType::Water;

        waterObject->renderItem->Model = ServiceProvider::getRenderResource()->mModels["watergrid"].get();
        waterObject->renderItem->MaterialOverwrite = ServiceProvider::getRenderResource()->mMaterials[entry["Material"]].get();
        waterObject->renderItem->renderType = RenderType::Water;
        waterObject->setTextureScale({ 2.5f,2.5f,2.5f });

        waterObject->updateTransforms();

        waterObject->renderItem->NumFramesDirty = gNumFrameResources;
        mGameObjects[waterObject->name] = std::move(waterObject);
    }


    return true;
}
