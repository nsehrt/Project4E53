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

    void update(float deltaTime);
    bool inProgress() const;
    bool fadingOn() const;
    bool fadingOff() const;
    float blur() const;
    float fade() const;

    private:

    TransitionStatus status = TransitionStatus::ComplOn;
    float transitionTimer = 0.0f;

    const float transitionTimeFull = 1.4f;
    const float fadeStartAt = 0.2f;
    const float blurUntil = 0.5f;
    const float maxBlur = 2.5f;
    const float maxFade = 1.0f;
    float blurValue = maxBlur;
    float fadeValue = maxFade;

};