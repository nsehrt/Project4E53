#include "../core/player.h"

#include "../core/level.h"
#include "../util/serviceprovider.h"

using namespace DirectX;

Player::Player(const std::string& model) : Character("Player", model, 0, 0)
{
    setPosition({ 0,0,0 });
    setAnimation(SP_ANIM("geo_Idle"));
    isFrustumCulled = false;
    setColliderProperties(GameCollider::GameObjectCollider::Sphere, { 0,0,0 }, { 0.25f,0.25f,0.25f });
}

void Player::update(const InputSet& input, const GameTime& gt)
{

    auto activeLevel = ServiceProvider::getActiveLevel();

    /*--- player movement*/
    XMFLOAT3 projectedPosition = getPosition();
    XMFLOAT2 inputVector = { input.current.trigger[TRG::THUMB_LX], input.current.trigger[TRG::THUMB_LY] };
    bool jumped = input.Pressed(BTN::A);
    bool running = input.current.trigger[TRG::RIGHT_TRIGGER] > 0.1f;
    bool rolled = input.Pressed(BTN::B);
    
    XMFLOAT2 inputDirection{};
    XMVECTOR inputDirectionV = XMVector2Normalize(XMLoadFloat2(&inputVector));
    XMStoreFloat2(&inputDirection, inputDirectionV);


    /*rotate player in left stick direction*/

    if (inputDirection.x != 0.0f && inputDirection.y != 0.0f)
    {
        /*reset idle timer while player is active*/
        timeIdle = 0.0f;
        timeMoving += gt.DeltaTime();

        /*do not instantly rotate to target rotation (input direction) but lerp there over time*/

        float targetRotation = MathHelper::angleFromXY(inputDirection.y, inputDirection.x) - MathHelper::Pi;//std::atan2f(inputDirection.x, inputDirection.y) - XM_PI;
        setRotation({ 0, MathHelper::lerpAngle(getRotation().y, targetRotation, turnSmoothTime * gt.DeltaTime()) ,0 });


        /*calculate movement speed*/

        auto inputMagnitudeV = XMVector2Length(XMLoadFloat2(&inputVector));
        float inputMagnitude{};
        XMStoreFloat(&inputMagnitude, inputMagnitudeV);

        /*use unnormalized inputMagnitude to have finer control over movement speed*/
        float currentSpeed = walkSpeed;

        if (running)
        {
            currentSpeed = runSpeed;

            currentCState = CharacterState::Run;
            if (previousCState != CharacterState::Run)
            {
                setAnimation(SP_ANIM("geo_Run"), true);
            }
        }
        else
        {
            currentCState = CharacterState::Walk;
            if (previousCState != CharacterState::Walk)
            {
                setAnimation(SP_ANIM("geo_Walk"), true);
            }
        }

        currentSpeed = currentSpeed * inputMagnitude;

        /*apply to calculated speed to the player position using the direction the player faces*/
        auto pPos = getPosition();
        auto pRot = MathHelper::vectorFromAngle(getRotation().y + XM_PIDIV2);

        pPos.x += pRot.x * currentSpeed * gt.DeltaTime();
        pPos.z += pRot.z * currentSpeed * gt.DeltaTime();

        projectedPosition = pPos;

    }
    else
    {
        /*reset idle timer when no input (standing still)*/
        currentCState = CharacterState::Idle;

        if (previousCState != CharacterState::Idle)
        {
            setAnimation(SP_ANIM("geo_Idle"), true);
        }

        timeIdle += gt.DeltaTime();
        timeMoving = 0.0f;

        if (timeIdle > SP_ANIM("geo_Idle")->getEndTime() * 4)
        {
            setAnimation(SP_ANIM("geo_Idle2"), false);
            timeIdle = 0.0f;
        }

    }

    /*players y position to terrain height*/
    projectedPosition = { projectedPosition.x, activeLevel->mTerrain->getHeight(projectedPosition.x, projectedPosition.z), projectedPosition.z };


    XMFLOAT3 currentPosition = getPosition();
    setPosition(projectedPosition);

    if (activeLevel->playerCollides())
    {
        setPosition(currentPosition);
    }

    /*TEMP*/
    GameObject::update(gt);


    

    previousCState = currentCState;

}
