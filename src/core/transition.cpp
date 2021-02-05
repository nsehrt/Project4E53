#include "transition.h"
#include "../util/serviceprovider.h"

void Transition::start()
{

    // on completed transition
    if(status == TransitionStatus::ComplOn)
    {
        status = TransitionStatus::TrOff;
        transitionTimer = 0.0f;
    }
    else if(status == TransitionStatus::ComplOff)
    {
        status = TransitionStatus::TrOn;
        transitionTimer = 0.0f;
    }

    // in progress switch
    else if(status == TransitionStatus::TrOn)
    {
        status = TransitionStatus::TrOff;
        transitionTimer = transitionTimeFull - transitionTimer;
    }
    else if(status == TransitionStatus::TrOff)
    {
        status = TransitionStatus::TrOn;
        transitionTimer = transitionTimeFull - transitionTimer;
    }
}

void Transition::update(float deltaTime)
{
    if(!inProgress())
    {
        return;
    }

    transitionTimer += deltaTime;

    // transition is done
    if(transitionTimer >= transitionTimeFull)
    {

        if(status == TransitionStatus::TrOn)
        {
            blurValue = maxBlur;
            fadeValue = maxFade;
            status = TransitionStatus::ComplOn;
            transitionTimer = 0.0f;
        }
        else if(status == TransitionStatus::TrOff)
        {
            blurValue = 0.0f;
            fadeValue = 0.0f;
            status = TransitionStatus::ComplOff;
            transitionTimer = 0.0f;
        }

    }

    //in between interpolation
    else
    {
        if(status == TransitionStatus::TrOff)
        {
            blurValue = transitionTimer > (transitionTimeFull - blurUntil) ?
                (1 - (transitionTimer - blurUntil) / (transitionTimeFull - blurUntil)) * maxBlur :
                maxBlur;
            fadeValue = transitionTimer < (transitionTimeFull - fadeStartAt) ?
                (1 - (transitionTimer / (transitionTimeFull - fadeStartAt))) * maxFade :
                0.0f;
                
        }
        else if(status == TransitionStatus::TrOn)
        {

            blurValue = transitionTimer < blurUntil ?
                (transitionTimer / blurUntil) * maxBlur :
                maxBlur;
            fadeValue = transitionTimer > fadeStartAt ?
                (transitionTimer - fadeStartAt) / (transitionTimeFull - fadeStartAt) * maxFade :
                0.0f;
        }

    }


}

bool Transition::inProgress() const
{
    return status == TransitionStatus::TrOff || status == TransitionStatus::TrOn;
}

bool Transition::fadingOn() const
{
    return status == TransitionStatus::TrOn;
}

bool Transition::fadingOff() const
{
    return status == TransitionStatus::TrOff;
}

float Transition::blur() const
{
    return blurValue;
}

float Transition::blurNormalized() const
{
    return blurValue / maxBlur;
}

float Transition::fade() const
{
    return fadeValue;
}
