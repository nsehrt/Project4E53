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

    /*parse grass*/
    if (!parseGrass(levelJson["Grass"]))
    {
        LOG(Severity::Critical, "Failed to load grass!");
        return false;
    }

    /*parse particle systems*/
    if (!parseParticleSystems(levelJson["ParticleSystem"]))
    {
        LOG(Severity::Critical, "Failed to load particle systems!");
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

    /*parse game objects*/
    if (!parseGameObjects(levelJson["GameObject"]))
    {
        LOG(Severity::Critical, "Failed to load Game Objects!");
        return false;
    }

    /*add test dynamic object*/

    auto testObject = std::make_unique<GameObject>(std::string("test"), amountObjectCBs++, 0);

    testObject->setSkinnedModel(ServiceProvider::getRenderResource()->mSkinnedModels["geo"].get(), ServiceProvider::getRenderResource()->mAnimations["metarig_metarigAction"].get(), 0);
    testObject->setPosition({ 0.0f,3.0f,0.0f });
    mGameObjects[testObject->Name] = std::move(testObject);

    auto testObject2 = std::make_unique<GameObject>(std::string("test2"), amountObjectCBs++, 1);
    testObject2->setSkinnedModel(ServiceProvider::getRenderResource()->mSkinnedModels["model"].get(), ServiceProvider::getRenderResource()->mAnimations["model_Animation"].get(), 1);
    testObject2->setPosition({ 5.0f,3.0f,0.0f });

    mGameObjects[testObject2->Name] = std::move(testObject2);



    calculateRenderOrderSizes();

    auto endTime = std::chrono::system_clock::now();
    std::chrono::duration<double> elapsedTime = endTime - startTime;

    LOG(Severity::Info, "Loaded level " << levelFile << " successfully. (" << elapsedTime.count() << " seconds, " << mGameObjects.size() << " GameObjects, " << amountObjectCBs << " ObjectCBs)");

    loadedLevel = LEVEL_PATH + std::string("/") + levelFile;

    return true;
}

void Level::update(const GameTime& gt)
{
    auto renderResource = ServiceProvider::getRenderResource();
    auto aCamera = ServiceProvider::getActiveCamera();

    XMMATRIX invView = XMMatrixInverse(&XMMatrixDeterminant(aCamera->getView()), aCamera->getView());

    BoundingFrustum localSpaceFrustum;
    aCamera->getFrustum().Transform(localSpaceFrustum, invView);

    /*udpdate all game objects*/
    for (auto& gameObj : mGameObjects)
    {
        if (gameObj.second->gameObjectType == GameObjectType::Dynamic)
        {
            int i = 1;
        }

        gameObj.second->checkInViewFrustum(localSpaceFrustum);
        gameObj.second->update(gt);
    }

    /*update order of light objects*/

    const auto cameraPos = aCamera->getTarget();

    /*order point lights by shortest distance from camera*/
    if (ServiceProvider::getSettings()->miscSettings.EditModeEnabled)
    {
        auto lPtr = mLightObjects[ServiceProvider::getEditSettings()->currentLightSelectionIndex].get();

        std::sort(mLightObjects.begin() + AMOUNT_DIRECTIONAL,
                  mLightObjects.end() - AMOUNT_SPOT,
                  [&](const std::unique_ptr<LightObject>& a, const std::unique_ptr<LightObject>& b)
                  {
                      XMVECTOR lengthA = XMVector3LengthSq(XMLoadFloat3(&a->getPosition()) - cameraPos);
                      XMVECTOR lengthB = XMVector3LengthSq(XMLoadFloat3(&b->getPosition()) - cameraPos);

                      XMFLOAT3 t, s;
                      XMStoreFloat3(&t, lengthA);
                      XMStoreFloat3(&s, lengthB);

                      return t.x < s.x;
                  });

        for (UINT i = 3; i < mLightObjects.size() - 1; i++)
        {
            if (mLightObjects[i].get() == lPtr)
            {
                ServiceProvider::getEditSettings()->currentLightSelectionIndex = i;
                ServiceProvider::getEditSettings()->currentLightSelectionPointIndex = i;
                break;
            }
        }

    }
    else
    {
        std::sort(mLightObjects.begin() + AMOUNT_DIRECTIONAL,
                  mLightObjects.end() - AMOUNT_SPOT,
                  [&](const std::unique_ptr<LightObject>& a, const std::unique_ptr<LightObject>& b)
                  {
                      XMVECTOR lengthA = XMVector3LengthSq(XMLoadFloat3(&a->getPosition()) - cameraPos);
                      XMVECTOR lengthB = XMVector3LengthSq(XMLoadFloat3(&b->getPosition()) - cameraPos);

                      XMFLOAT3 t, s;
                      XMStoreFloat3(&t, lengthA);
                      XMStoreFloat3(&s, lengthB);

                      return t.x < s.x;
                  });
    }



    for (UINT i = 3; i < (MAX_LIGHTS - 1); i++)
    {
        if (i < mLightObjects.size() - 1)
            mCurrentLightObjects[i] = mLightObjects[i].get();
    }

    /*water*/
    for (auto& w : mWater)
    {
        w->update(gt);
    }

    /*particle systems*/
for (auto& p : mParticleSystems)
{
    if (mGameObjects[p.first]->getIsInFrustum())
    {
        p.second->update(gt);
    }
}

/*TEST Collision test*/
if (!ServiceProvider::getSettings()->miscSettings.EditModeEnabled)
for (const auto& gameObj : mGameObjects)
{
    if (gameObj.second->gameObjectType != GameObjectType::Static &&
        gameObj.second->gameObjectType != GameObjectType::Wall) continue;

    if (gameObj.second->intersectsRough(ServiceProvider::getActiveCamera()->getBoundingBox()))
    {
        LOG(Severity::Info, "Camera collided with " << gameObj.second->Name);
        //ServiceProvider::getActiveCamera()->setPosition(ServiceProvider::getActiveCamera()->mPreviousPosition);
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

    /*sort the transparent objects by distance from camera*/
    const auto aCamera = ServiceProvider::getActiveCamera();
    const auto cameraPos = aCamera->getPosition();

    std::sort(std::begin(renderOrder[(int)RenderType::DefaultTransparency]),
              std::end(renderOrder[(int)RenderType::DefaultTransparency]),
              [&](const GameObject* a, const GameObject* b)
              {
                  XMVECTOR lengthA = XMVector3LengthSq(XMLoadFloat3(&a->getPosition()) - cameraPos);
                  XMVECTOR lengthB = XMVector3LengthSq(XMLoadFloat3(&b->getPosition()) - cameraPos);

                  XMFLOAT3 t, s;
                  XMStoreFloat3(&t, lengthA);
                  XMStoreFloat3(&s, lengthB);

                  return t.x > s.x;
              });

    // draw the gameobjects
    UINT objectsDrawn = 0;


    for (UINT i = 0; i < renderOrder.size(); i++)
    {
        if (renderOrder[i].empty())continue;
        if (i == (UINT)RenderType::Terrain) continue;

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
        renderResource->setPSO(PostProcessRenderType::Hitbox);

        for (const auto& gameObject : mGameObjects)
        {
            auto g = gameObject.second.get();

            if (g->renderItem->renderType != RenderType::Sky)
            {
                gameObject.second->drawRoughHitbox();
            }
        }
    }

    /*draw outlined obj if needed*/
    if (ServiceProvider::getSettings()->miscSettings.EditModeEnabled &&
        ServiceProvider::getEditSettings()->toolMode == EditTool::Camera &&
        ServiceProvider::getEditSettings()->currentSelection != nullptr &&
        ServiceProvider::getEditSettings()->currentSelection->gameObjectType == GameObjectType::Static)
    {
        renderResource->setPSO(PostProcessRenderType::Outline);
        ServiceProvider::getEditSettings()->currentSelection->draw();
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

    }

    /*gameobject*/

    for (const auto& e : mGameObjects)
    {
        if (e.second->gameObjectType != GameObjectType::Static) continue;

       saveFile["GameObject"].push_back(e.second->toJson());
    }

    /*grass*/
    UINT c = 0;

    for (const auto& e : mGrass)
    {
        saveFile["Grass"].push_back(e->toJson());

        saveFile["Grass"][c]["Position"][0] = mGameObjects[e->getName()]->getPosition().x;
        saveFile["Grass"][c]["Position"][2] = mGameObjects[e->getName()]->getPosition().z;

        saveFile["Grass"][c]["Size"][0] = mGameObjects[e->getName()]->getScale().x * e->getSize().x;
        saveFile["Grass"][c]["Size"][1] = mGameObjects[e->getName()]->getScale().z * e->getSize().y;

        saveFile["Grass"][c]["Density"][0] = (int)(mGameObjects[e->getName()]->getScale().x * (e->getSize().x / e->getQuadSize()) * 2.5f);
        saveFile["Grass"][c]["Density"][1] = (int)(mGameObjects[e->getName()]->getScale().z * (e->getSize().y / e->getQuadSize()) * 3.5f);

        c++;
    }


    /*water*/
    for (auto& w : mWater)
    {
        w->isSaved = false;
    }

    for (const auto& e : mGameObjects)
    {

        if (e.second->gameObjectType == GameObjectType::Water)
        {
            json wElement;

            wElement["Name"] = e.second->Name;

            wElement["Position"][0] = e.second->getPosition().x;
            wElement["Position"][1] = e.second->getPosition().y;
            wElement["Position"][2] = e.second->getPosition().z;

            wElement["Scale"][0] = e.second->getScale().x;
            wElement["Scale"][1] = e.second->getScale().y;
            wElement["Scale"][2] = e.second->getScale().z;

            wElement["Rotation"][0] = XMConvertToDegrees(e.second->getRotation().x);
            wElement["Rotation"][1] = XMConvertToDegrees(e.second->getRotation().y);
            wElement["Rotation"][2] = XMConvertToDegrees(e.second->getRotation().z);

            wElement["TexScale"][0] = e.second->getTextureScale().x;
            wElement["TexScale"][1] = e.second->getTextureScale().y;
            wElement["TexScale"][2] = e.second->getTextureScale().z;

            wElement["Material"] = e.second->renderItem->MaterialOverwrite->Name;

            for (auto& w : mWater)
            {
                if (wElement["Material"] == w->getName())
                {
                    if (!w->isSaved)
                    {
                        w->addPropertiesToJson(wElement);
                        w->isSaved = true;
                    }
                }
            }

            saveFile["Water"].push_back(wElement);
        }

    }


    /*particle system*/
    c = 0;

    for (const auto& e : mParticleSystems)
    {
        saveFile["ParticleSystem"].push_back(e.second->toJson());
        saveFile["ParticleSystem"][c]["Position"] = {
            mGameObjects[saveFile["ParticleSystem"][c]["Name"]]->getPosition().x,
            mGameObjects[saveFile["ParticleSystem"][c]["Name"]]->getPosition().y,
            mGameObjects[saveFile["ParticleSystem"][c]["Name"]]->getPosition().z
        };

        c++;
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
    newLevel["Terrain"]["BlendTextures"][2] = "PavementGray.dds";
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

    light4["Name"] = "EditLight";
    light4["Position"][0] = 0.0f;
    light4["Position"][1] = -20.0f;
    light4["Position"][2] = 0.0f;
    light4["Strength"][0] = 1.0f;
    light4["Strength"][1] = 1.0f;
    light4["Strength"][2] = 1.0f;
    light4["FallOffStart"] = 1.0f;
    light4["FallOffEnd"] = 1.1f;

    newLevel["Light"]["Point"].push_back(light4);

    json light5;

    light5["Name"] = "P_(1)";
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
    gameObj["Position"] = { 0.0f, 4.0f, 0.0f };

    newLevel["GameObject"].push_back(gameObj);

    json invWallObj;

    invWallObj["Name"] = "invWall";
    invWallObj["RenderType"] = "DefaultTransparency";
    invWallObj["Material"] = "invWall";
    invWallObj["Model"] = "";
    invWallObj["DrawEnabled"] = false;
    invWallObj["ShadowEnabled"] = false;
    invWallObj["Position"] = { -4.0f,4.0f,0.0f };

    newLevel["GameObject"].push_back(invWallObj);

    json grassObj;

    grassObj["Name"] = "grass";
    grassObj["Material"] = "grass_3";
    grassObj["Position"] = { 0.0f,0.0f,0.0f };
    grassObj["QuadSize"] = { 1.0f,1.0f };
    grassObj["Size"] = { 5.0f,5.0f };
    grassObj["SizeVariation"] = 0.0f;
    grassObj["Density"] = { 16,16 };

    newLevel["Grass"].push_back(grassObj);

    json waterObj;

    waterObj["Name"] = "water";
    waterObj["Material"] = "water";
    waterObj["Displacement1Transform"] = { 0.8f,0.01f,0.03f };
    waterObj["Displacement2Transform"] = { 0.4f,-0.01f,0.03f };
    waterObj["Normal1Transform"] = { 5.0f,0.05f,0.2f };
    waterObj["Normal2Transform"] = { 4.0f,-0.02f,0.05f };
    waterObj["Position"] = { 10.0f,0.0f,0.0f };
    waterObj["Rotation"] = { 0.0f,0.0f,0.0f };
    waterObj["Scale"] = { 1.0f,1.0f,1.0f };
    waterObj["TexScale"] = { 10.0f,10.0f,1.0f };
    waterObj["HeightScale"] = { 0.4f,0.8f };
    waterObj["MaterialTranslation"] = {0.15f,0.4f };

    newLevel["Water"].push_back(waterObj);

    json particleObj;

    particleObj["Name"] = "particle_fire";
    particleObj["Material"] = "fire1";
    particleObj["MaxAge"] = 1.0f;
    particleObj["ParticleCount"] = 250;
    particleObj["Size"] = { 1.0f,1.0f };
    particleObj["SpawnTime"] = 0.005;
    particleObj["Type"] = "Fire";
    particleObj["DirectionMultiplier"] = { 0.7f, 1.3f, 0.7f };
    particleObj["Position"] = { 0.0, 3.0f, 5.0f };

    newLevel["ParticleSystem"].push_back(particleObj);

    json particleObj2;

    particleObj2["Name"] = "particle_smoke";
    particleObj2["Material"] = "smoke1";
    particleObj2["MaxAge"] = 6.0f;
    particleObj2["ParticleCount"] = 250;
    particleObj2["Size"] = { 1.0f,1.0f };
    particleObj2["SpawnTime"] = 0.05;
    particleObj2["Type"] = "Smoke";
    particleObj2["DirectionMultiplier"] = { 0.1f, 0.4f, 0.1f };
    particleObj2["Position"] = { 2.0, 3.0f, 5.0f };

    newLevel["ParticleSystem"].push_back(particleObj2);

    /*write to file*/
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

    /*draw shadows*/
    for (UINT i = 0; i < shadowRenderOrder.size(); i++)
    {
        if (shadowRenderOrder[i].empty()) continue;

        renderResource->setPSO(ShadowRenderType((int)ShadowRenderType::ShadowDefault + i));

        for (const auto& gameObject : shadowRenderOrder[i])
        {
            if (gameObject->intersectsShadowBounds(renderResource->getShadowMap()->shadowBounds))
            {
                objectsDrawn += gameObject->drawShadow();
            }
        }
    }

    ServiceProvider::getDebugInfo()->DrawnShadowObjects = objectsDrawn;
}

bool Level::existsLightByName(const std::string& name)
{
    for (const auto& l : mLightObjects)
    {
        if (l->name == name)
            return true;
    }

    return false;
}

void Level::calculateRenderOrderSizes()
{
    /*determine size of render orders*/
    std::vector<int> renderOrderSize((int)RenderType::COUNT);
    std::vector<int> shadowRenderOrderSize((int)ShadowRenderType::COUNT);

    renderOrder.clear();
    shadowRenderOrder.clear();

    for (const auto& gameOject : mGameObjects)
    {
        renderOrderSize[(int)gameOject.second->renderItem->renderType]++;
        shadowRenderOrderSize[(int)gameOject.second->renderItem->shadowType]++;
    }

    for (int i = 0; i < renderOrderSize.size(); i++)
    {
        renderOrder.push_back(std::vector<GameObject*>(renderOrderSize[i]));
    }

    for (int i = 0; i < shadowRenderOrderSize.size(); i++)
    {
        shadowRenderOrder.push_back(std::vector<GameObject*>(shadowRenderOrderSize[i]));
    }

    calculateRenderOrder();
    calculateShadowRenderOrder();
}

void Level::calculateRenderOrder()
{
    /* Order of render items*/
    for (auto& v : renderOrder)
    {
        v.clear();
    }

    for (const auto& gameObject : mGameObjects)
    {
        renderOrder[(int)gameObject.second->renderItem->renderType].push_back(&(*gameObject.second));
    }
}

void Level::calculateShadowRenderOrder()
{
    /*order shadow render order*/
    for (auto& v : shadowRenderOrder)
    {
        v.clear();
    }

    for (const auto& gameObject : mGameObjects)
    {
        if (gameObject.second->renderItem->renderType == RenderType::Sky ||
            gameObject.second->gameObjectType == GameObjectType::Debug ||
            gameObject.second->renderItem->renderType == RenderType::Terrain) continue;

        shadowRenderOrder[(long long)gameObject.second->renderItem->shadowType].push_back(&(*gameObject.second));
    }
}

bool Level::existsList(const nlohmann::json& j, const std::vector<std::string>& key)
{
    for (const auto& k : key)
    {
        if (!exists(j, k))
        {
            LOG(Severity::Error, "Missing property " << k << "!");
            return false;
        }
    }

    return true;
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
    rItem->staticModel = renderResource->mModels["sphere"].get();
    rItem->renderType = RenderType::Sky;

    gameObject->Name = "SKY_SPHERE";
    gameObject->renderItem = std::move(rItem);
    gameObject->gameObjectType = GameObjectType::Sky;
    gameObject->isFrustumCulled = false;

    mGameObjects[gameObject->Name] = std::move(gameObject);

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
    json spotJson;

    spotJson["Name"] = "spotlight";
    spotJson["Position"] = { 0.0f,-2.0f,0.0f };
    spotJson["Direction"] = { 0.0f, -1.0f, 0.0f };
    spotJson["Strength"] = { 1.0f,1.0f,1.0f };
    spotJson["SpotPower"] = 32.0f;
    spotJson["FallOffStart"] = 5.0f;
    spotJson["FallOffEnd"] = 12.0f;

    auto lightObj = std::make_unique<LightObject>(LightType::Spot, spotJson);
    mLightObjects.push_back(std::move(lightObj));


    /*set immutable lights*/

    /*directional*/
    mCurrentLightObjects[0] = mLightObjects[0].get();
    mCurrentLightObjects[1] = mLightObjects[1].get();
    mCurrentLightObjects[2] = mLightObjects[2].get();

    /*spot*/
    mCurrentLightObjects[7] = mLightObjects[mLightObjects.size() - 1].get();

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
        amountObjectCBs += 4;

        mGameObjects[gameObject->Name] = std::move(gameObject);
    }

    auto debugObject = std::make_unique<GameObject>(std::string("DEBUG"), amountObjectCBs++);

    debugObject->isFrustumCulled = false;
    debugObject->isShadowEnabled = false;
    debugObject->renderItem->renderType = RenderType::Default;
    debugObject->gameObjectType = GameObjectType::Debug;
    debugObject->renderItem->staticModel = ServiceProvider::getRenderResource()->mModels["quad"].get();

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
    auto terrainObject = std::make_unique<GameObject>(std::string("TERRAIN"), amountObjectCBs++);

    terrainObject->isFrustumCulled = false;
    terrainObject->isShadowEnabled = false;
    terrainObject->isDrawEnabled = true;
    terrainObject->isCollisionEnabled = false;
    terrainObject->renderItem->renderType = RenderType::Terrain;
    terrainObject->gameObjectType = GameObjectType::Terrain;
    terrainObject->renderItem->staticModel = mTerrain->terrainModel.get();
    terrainObject->renderItem->MaterialOverwrite = ServiceProvider::getRenderResource()->mMaterials["terrain"].get();
    XMStoreFloat4x4(&terrainObject->renderItem->TexTransform, XMMatrixScaling((float)mTerrain->terrainSlices / 4.0f,
                    (float)mTerrain->terrainSlices / 4.0f, (float)mTerrain->terrainSlices / 4.0f));

    mGameObjects["TERRAIN"] = std::move(terrainObject);

    return true;
}

bool Level::parseGrass(const json& grassJson)
{

    std::vector<std::string> checklist{ "Name", "Size", "Position", "Material",
                                    "Density", "QuadSize", "SizeVariation" };

    for (const auto& entry : grassJson)
    {
        /*check validity*/
        if (!existsList(entry, checklist))
        {
            continue;
        }

        if (mGameObjects.find(entry["Name"]) != mGameObjects.end())
        {
            LOG(Severity::Warning, "GameObject " << entry["Name"] << " already exists!");
            continue;
        }

        auto grass = std::make_unique<Grass>(ServiceProvider::getRenderResource());

        grass->create(entry, mTerrain.get());

        mGrass.push_back(std::move(grass));

        auto grassObject = std::make_unique<GameObject>(mGrass.back()->getName(), amountObjectCBs++);

        grassObject->isFrustumCulled = true;
        grassObject->isShadowEnabled = false;
        grassObject->isDrawEnabled = true;
        grassObject->isCollisionEnabled = false;
        grassObject->gameObjectType = GameObjectType::Grass;
        grassObject->renderItem->renderType = RenderType::Grass;
        grassObject->renderItem->staticModel = mGrass.back()->getPatchModel();
        grassObject->renderItem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_POINTLIST;
        grassObject->renderItem->MaterialOverwrite = ServiceProvider::getRenderResource()->mMaterials[mGrass.back()->getMaterialName()].get();
        grassObject->setPosition(mGrass.back()->getPosition());
        grassObject->setFrustumHitBoxExtents(XMFLOAT3(mGrass.back()->getSize().x, mGrass.back()->getHighestPoint(), mGrass.back()->getSize().y));


        mGameObjects[mGrass.back()->getName()] = std::move(grassObject);

    }

    return true;
}

bool Level::parseWater(const json& waterJson)
{
    std::vector<std::string> checklist{ "Name", "Material", "Position", "Scale",
                                        "Rotation", "TexScale" };

    for (const auto& entry : waterJson)
    {
        /*check validity*/
        if (!existsList(entry, checklist))
        {
            continue;
        }

        if (mGameObjects.find(entry["Name"]) != mGameObjects.end())
        {
            LOG(Severity::Warning, "GameObject " << entry["Name"] << " already exists!");
            continue;
        }

        auto waterObject = std::make_unique<GameObject>(std::string(entry["Name"]), amountObjectCBs++);
        XMFLOAT3 pos, scale, rot;

        pos.x = entry["Position"][0];
        pos.y = entry["Position"][1];
        pos.z = entry["Position"][2];
        waterObject->setPosition(pos);

        scale.x = entry["Scale"][0];
        scale.y = entry["Scale"][1];
        scale.z = entry["Scale"][2];
        waterObject->setScale(scale);

        rot.x = XMConvertToRadians(entry["Rotation"][0]);
        rot.y = XMConvertToRadians(entry["Rotation"][1]);
        rot.z = XMConvertToRadians(entry["Rotation"][2]);
        waterObject->setRotation(rot);

        waterObject->isShadowEnabled = false;
        waterObject->isCollisionEnabled = false;
        waterObject->isShadowForced = false;
        waterObject->isFrustumCulled = true;
        waterObject->gameObjectType = GameObjectType::Water;

        waterObject->renderItem->staticModel = ServiceProvider::getRenderResource()->mModels["watergrid"].get();
        waterObject->renderItem->MaterialOverwrite = ServiceProvider::getRenderResource()->mMaterials[entry["Material"]].get();
        waterObject->renderItem->renderType = RenderType::Water;
        waterObject->renderItem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST;
        waterObject->setTextureScale({ entry["TexScale"][0],
                                       entry["TexScale"][1], 
                                       entry["TexScale"][2] });

        waterObject->updateTransforms();

        waterObject->renderItem->NumFramesDirty = gNumFrameResources;
        mGameObjects[waterObject->Name] = std::move(waterObject);

        /*create water material updater if needed*/

        bool needed = true;

        for (const auto& w : mWater)
        {
            if (w->getName() == entry["Material"])
            {
                needed = false;
                break;
            }
        }

        if (needed)
        {
            mWater.push_back(std::make_unique<Water>(ServiceProvider::getRenderResource(), entry));
        }

    }


    return true;
}

bool Level::parseParticleSystems(const json& particleJson)
{
    std::vector<std::string> checklist{ "Name", "Material", "Position", "Size",
                                    "Type", "MaxAge", "SpawnTime", "DirectionMultiplier" };

    UINT indexCounter = 0;

    for (const auto& entry : particleJson)
    {

        if (!existsList(entry, checklist))
        {
            continue;
        }

        if (mGameObjects.find(entry["Name"]) != mGameObjects.end())
        {
            LOG(Severity::Warning, "GameObject " << entry["Name"] << " already exists!");
            continue;
        }

        /*init particle system*/
        mParticleSystems[entry["Name"]] = std::make_unique<ParticleSystem>(ServiceProvider::getRenderResource(), indexCounter++);
        mParticleSystems[entry["Name"]]->init(entry);

        /*create game object*/
        auto particleObject = std::make_unique<GameObject>(std::string(entry["Name"]), amountObjectCBs++);

        particleObject->isFrustumCulled = true;
        particleObject->isShadowEnabled = false;
        particleObject->isDrawEnabled = true;
        particleObject->isCollisionEnabled = true;
        particleObject->gameObjectType = GameObjectType::Particle;

        switch (mParticleSystems[entry["Name"]]->getType())
        {
            case ParticleSystemType::Fire: particleObject->renderItem->renderType = RenderType::Particle_Fire; break;
            case ParticleSystemType::Smoke: particleObject->renderItem->renderType = RenderType::Particle_Smoke; break;
            default: particleObject->renderItem->renderType = RenderType::Particle_Fire; break;
        }

        particleObject->renderItem->staticModel = mParticleSystems[entry["Name"]]->getModel();
        particleObject->renderItem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_POINTLIST;
        particleObject->renderItem->MaterialOverwrite = ServiceProvider::getRenderResource()->mMaterials[mParticleSystems[entry["Name"]]->getMaterialName()].get();
        particleObject->setFrustumHitBoxExtents(mParticleSystems[entry["Name"]]->getRoughDimensions());
        particleObject->setPosition(mParticleSystems[entry["Name"]]->getPosition());

        mGameObjects[entry["Name"]] = std::move(particleObject);
    }


    return true;
}
