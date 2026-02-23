#include "Rendering/VisualComponent.h"
#include "Engine/Log.h"

void VisualComponent::playAnimation(const SkelAnimation* animation_, const SkelAnimation::Frame* initialFrame_)
{
    if (animation_)
    {
        animation = animation_;
        initialFrame = initialFrame_;
        assert(animation->getFramerate() > 0.0f);
        time = 0.0f;
        isAnimationPlaying = true;
    }
}

void VisualComponent::stopAnimation()
{
    isAnimationPlaying = false;
}

const SkelAnimation::Frame* VisualComponent::getAnimationFrame() const
{
    if (!animation)
        return nullptr;
    
    float framerate = animation->getFramerate();
    uint numFrames = animation->getNumFrames();
    int frameIndex = (int)((time / 1000) * framerate);
    frameIndex = frameIndex % numFrames;
    return &animation->getFrame(frameIndex);
}
void VisualComponent::tick(float dt)
{
    if (animation && isAnimationPlaying)
    {
        time += dt * animationSpeed;
    }
}