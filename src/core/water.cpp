#include "water.h"

Water::Water(RenderResource* r, const json& waterJson)
{

    material = r->mMaterials[waterJson["Material"]].get();
    materialName = waterJson["Material"];

    materialTranslation.x = waterJson["MaterialTranslation"][0];
    materialTranslation.y = waterJson["MaterialTranslation"][1];

    displacement1Translation.x = waterJson["Displacement1Translation"][0];
    displacement1Translation.y = waterJson["Displacement1Translation"][1];

    displacement2Translation.x = waterJson["Displacement2Translation"][0];
    displacement2Translation.y = waterJson["Displacement2Translation"][1];


    displacement1Scale.x = waterJson["Displacement1Scale"][0];
    displacement1Scale.y = waterJson["Displacement1Scale"][1];
    displacement1Scale.z = waterJson["Displacement1Scale"][2];

    displacement2Scale.x = waterJson["Displacement2Scale"][0];
    displacement2Scale.y = waterJson["Displacement2Scale"][1];
    displacement2Scale.z = waterJson["Displacement2Scale"][2];

    /*random offset so dont all materials get updated on the same frame*/
    updateTime = getRandomFloat(0.0f, updFixedTime);

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

        material->MatTransform(0, 0) = 10;
        material->MatTransform(1, 1) = 10;
        material->MatTransform(2, 2) = 1;

        /*displacement 1*/
        tu = material->DisplacementTransform0(3, 0);
        tv = material->DisplacementTransform0(3, 1);

        tu += displacement1Translation.x * updFixedTime;
        tv += displacement1Translation.y * updFixedTime;

        border(tu); border(tv);

        XMStoreFloat4x4(&material->DisplacementTransform0,
                        XMMatrixScaling(displacement1Scale.x,displacement1Scale.y, displacement1Scale.z) * XMMatrixTranslation(tu, 0.0f, tv));

        /*displacement 2*/
        tu = material->DisplacementTransform1(3, 0);
        tv = material->DisplacementTransform1(3, 1);

        tu += displacement2Translation.x * updFixedTime;
        tv += displacement2Translation.y * updFixedTime;

        border(tu); border(tv);

        XMStoreFloat4x4(&material->DisplacementTransform1,
                        XMMatrixScaling(displacement2Scale.x, displacement2Scale.y, displacement2Scale.z) * XMMatrixTranslation(tu, 0.0f, tv));


        /*finish*/
        material->NumFramesDirty = gNumFrameResources;

        updateTime -= updFixedTime;
    }

}

void Water::addPropertiesToJson(json& wJson)
{

    wJson["MaterialTranslation"][0] = materialTranslation.x;
    wJson["MaterialTranslation"][1] = materialTranslation.y;

    wJson["Displacement1Scale"][0] = displacement1Scale.x;
    wJson["Displacement1Scale"][1] = displacement1Scale.y;
    wJson["Displacement1Scale"][2] = displacement1Scale.z;

    wJson["Displacement2Scale"][0] = displacement2Scale.x;
    wJson["Displacement2Scale"][1] = displacement2Scale.y;
    wJson["Displacement2Scale"][2] = displacement2Scale.z;

    wJson["Displacement1Translation"][0] = displacement1Translation.x;
    wJson["Displacement1Translation"][1] = displacement1Translation.y;

    wJson["Displacement2Translation"][0] = displacement2Translation.x;
    wJson["Displacement2Translation"][1] = displacement2Translation.y;
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
