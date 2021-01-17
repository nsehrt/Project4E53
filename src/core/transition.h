#pragma once

enum class TransitionStatus
{
    ComplOn,
    ComplOff,
    TrOn,
    TrOff
};

class Transition
{


    public:

    explicit Transition() = default;

    void start();


    private:

    TransitionStatus status = TransitionStatus::ComplOff;
    float blurValue = 0.0f;
    float fadeValue = 0.0f;


};