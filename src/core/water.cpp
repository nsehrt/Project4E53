#include "water.h"
#include "../util/serviceprovider.h"
#include "../util/randomizer.h"

using namespace DirectX;

Water::Water(RenderResource* r, const json& waterJson)
{

    material = r->mMaterials[waterJson["Material"]].get();
    materialName = waterJson["Material"];

    materialTranslation.x = waterJson["MaterialTranslation"][0];
    materialTranslation.y = waterJson["MaterialTranslation"][1];

    displacement1Scale.x = waterJson["Displacement1Transform"][0];
    displacement1Scale.y = waterJson["Displacement1Transform"][0];
    displacement1Scale.z = waterJson["Displacement1Transform"][0];
    displacement1Translation.x = waterJson["Displacement1Transform"][1];
    displacement1Translation.y = waterJson["Displacement1Transform"][2];


    displacement2Scale.x = waterJson["Displacement2Transform"][0];
    displacement2Scale.y = waterJson["Displacement2Transform"][0];
    displacement2Scale.z = waterJson["Displacement2Transform"][0];
    displacement2Translation.x = waterJson["Displacement2Transform"][1];
    displacement2Translation.y = waterJson["Displacement2Transform"][2];

    normal1Scale.x = waterJson["Normal1Transform"][0];
    normal1Scale.y = waterJson["Normal1Transform"][0];
    normal1Scale.z = waterJson["Normal1Transform"][0];
    normal1Translation.x = waterJson["Normal1Transform"][1];
    normal1Translation.y = waterJson["Normal1Transform"][2];

    normal2Scale.x = waterJson["Normal2Transform"][0];
    normal2Scale.y = waterJson["Normal2Transform"][0];
    normal2Scale.z = waterJson["Normal2Transform"][0];
    normal2Translation.x = waterJson["Normal2Transform"][1];
    normal2Translation.y = waterJson["Normal2Transform"][2];

    matScale.x = waterJson["TexScale"][0];
    matScale.y = waterJson["TexScale"][1];
    matScale.z = waterJson["TexScale"][2];

    heightScale.x = waterJson["HeightScale"][0];
    heightScale.y = waterJson["HeightScale"][1];

    /*random offset so dont all materials get updated on the same frame*/
    updateTime = ServiceProvider::getRandomizer()->nextFloat(0.0f, updFixedTime);
        //MathHelper::randF(0.0f, updFixedTime);

}

void Water::update(const GameTime& gt)
{
    updateTime += gt.DeltaTime();

    if (updateTime >= updFixedTime)
    {
        /*material translation*/
        float tu = material->MatTransform(3, 0);
        float tv = material->MatTransform(3, 1);


        tu += materialTranslation.x * updFixedTime;
        tv += materialTranslation.y * updFixedTime;

        border(tu); border(tv);

        material->MatTransform(3, 0) = tu;
        material->MatTransform(3, 1) = tv;

        material->MatTransform(0, 0) = matScale.x;
        material->MatTransform(1, 1) = matScale.y;
        material->MatTransform(2, 2) = matScale.z;

        /*displacement 1*/
        tu = material->DisplacementTransform0(3, 0);
        tv = material->DisplacementTransform0(3, 1);

        tu += displacement1Translation.x * updFixedTime;
        tv += displacement1Translation.y * updFixedTime;

        border(tu); border(tv);

        XMStoreFloat4x4(&material->DisplacementTransform0,
                        XMMatrixScaling(displacement1Scale.x,displacement1Scale.y, displacement1Scale.z) * XMMatrixTranslation(tu, tv, 0.0f));

        /*displacement 2*/
        tu = material->DisplacementTransform1(3, 0);
        tv = material->DisplacementTransform1(3, 1);

        tu += displacement2Translation.x * updFixedTime;
        tv += displacement2Translation.y * updFixedTime;

        border(tu); border(tv);

        XMStoreFloat4x4(&material->DisplacementTransform1,
                        XMMatrixScaling(displacement2Scale.x, displacement2Scale.y, displacement2Scale.z) * XMMatrixTranslation(tu, tv, 0.0f));

        /*normal 1*/
        tu = material->NormalTransform0(3, 0);
        tv = material->NormalTransform0(3, 1);

        tu += normal1Translation.x * updFixedTime;
        tv += normal1Translation.y * updFixedTime;

        border(tu); border(tv);

        XMStoreFloat4x4(&material->NormalTransform0,
                        XMMatrixScaling(normal1Scale.x, normal1Scale.y, normal1Scale.z) * XMMatrixTranslation(tu, tv, 0.0f));

        /*normal 2*/
        tu = material->NormalTransform1(3, 0);
        tv = material->NormalTransform1(3, 1);

        tu += normal2Translation.x * updFixedTime;
        tv += normal2Translation.y * updFixedTime;

        border(tu); border(tv);

        XMStoreFloat4x4(&material->NormalTransform1,
                        XMMatrixScaling(normal2Scale.x, normal2Scale.y, normal2Scale.z) * XMMatrixTranslation(tu, tv, 0.0f));


        material->MiscFloat1 = heightScale.x;
        material->MiscFloat2 = heightScale.y;

        /*finish*/
        material->NumFramesDirty = gNumFrameResources;

        updateTime -= updFixedTime;
    }

}

void Water::addPropertiesToJson(json& wJson)
{

    wJson["MaterialTranslation"][0] = materialTranslation.x;
    wJson["MaterialTranslation"][1] = materialTranslation.y;

    wJson["Displacement1Transform"][0] = displacement1Scale.x;
    wJson["Displacement1Transform"][1] = displacement1Translation.x;
    wJson["Displacement1Transform"][2] = displacement1Translation.y;

    wJson["Displacement2Transform"][0] = displacement2Scale.x;
    wJson["Displacement2Transform"][1] = displacement2Translation.x;
    wJson["Displacement2Transform"][2] = displacement2Translation.y;

    wJson["Normal1Transform"][0] = normal1Scale.x;
    wJson["Normal1Transform"][1] = normal1Translation.x;
    wJson["Normal1Transform"][2] = normal1Translation.y;

    wJson["Normal2Transform"][0] = normal2Scale.x;
    wJson["Normal2Transform"][1] = normal2Translation.x;
    wJson["Normal2Transform"][2] = normal2Translation.y;

    wJson["HeightScale"][0] = heightScale.x;
    wJson["HeightScale"][1] = heightScale.y;
}

void Water::border(float& x)
{
    if (x > 1.0f)
        x -= 1.0f;

    if (x < 0.0f)
    {
        x += 1.0f;
    }
}
