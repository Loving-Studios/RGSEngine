#include "Time.h"
#include "Log.h"
#include <SDL3/SDL.h>
#include <algorithm>

// Initialize static variables
float Time::deltaTime = 0.0f;
float Time::time = 0.0f;
float Time::timeScale = 1.0f;
uint64_t Time::frameCount = 0;

float Time::realTimeSinceStartup = 0.0f;
float Time::realDeltaTime = 0.0f;

bool Time::isPaused = false;
bool Time::isStepFrame = false;

uint64_t Time::startTimeMS = 0;
uint64_t Time::lastFrameTimeMS = 0;
uint64_t Time::gameStartTimeMS = 0;

void Time::Init()
{
    startTimeMS = SDL_GetTicks();
    lastFrameTimeMS = startTimeMS;
    gameStartTimeMS = startTimeMS;

    deltaTime = 0.0f;
    time = 0.0f;
    timeScale = 1.0f;
    frameCount = 0;
    realTimeSinceStartup = 0.0f;
    realDeltaTime = 0.0f;
    isPaused = false;
    isStepFrame = false;

    LOG("Time Manager initialized");
}

void Time::Update()
{
    frameCount++;

    uint64_t currentTimeMS = SDL_GetTicks();
    uint64_t elapsedMS = currentTimeMS - lastFrameTimeMS;
    lastFrameTimeMS = currentTimeMS;

    realDeltaTime = elapsedMS / 1000.0f;
    realTimeSinceStartup = (currentTimeMS - startTimeMS) / 1000.0f;

    
    realDeltaTime = std::min(realDeltaTime, 0.1f);

    if (isPaused)
    {
        deltaTime = 0.0f;

        if (isStepFrame)
        {
            deltaTime = realDeltaTime * timeScale;
            time += deltaTime;
            isStepFrame = false;
        }
    }
    else
    {
        deltaTime = realDeltaTime * timeScale;
        time += deltaTime;
    }
}

void Time::Reset()
{
    // Reset Game Clock when you stop
    time = 0.0f;
    deltaTime = 0.0f;
    frameCount = 0;
    isPaused = false;
    isStepFrame = false;
    gameStartTimeMS = SDL_GetTicks();

    LOG("Game Clock reset");
}

void Time::SetTimeScale(float scale)
{
    
    timeScale = std::max(0.01f, std::min(scale, 10.0f));
    LOG("Time Scale set to: %.2fx", timeScale);
}

void Time::Pause()
{
    if (!isPaused)
    {
        isPaused = true;
        LOG("Game Clock PAUSED");
    }
}

void Time::Resume()
{
    if (isPaused)
    {
        isPaused = false;
        LOG("Game Clock RESUMED");
    }
}

void Time::Step()
{
    if (isPaused)
    {
        isStepFrame = true;
        LOG("Step: Advancing 1 frame");
    }
}