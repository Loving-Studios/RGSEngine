#pragma once

#include <cstdint>


class Time
{
public:
    // Game Clock
    static float deltaTime;          // Last frame time
    static float time;               // Total time since game start
    static float timeScale;         
    static uint64_t frameCount;      // Frames rendered from start

    // Real Time Clock
    static float realTimeSinceStartup;  
    static float realDeltaTime;

    // Internal State 
    static bool isPaused;               // If the game is paused
    static bool isStepFrame;            //  If we want to advance 1 frame

    // Methods
    static void Init();
    static void Update();              
    static void Reset();               // Reset when doing Stop

    // Control
    static void SetTimeScale(float scale);
    static void Pause();
    static void Resume();
    static void Step();                

private:
    // Internal timing
    static uint64_t startTimeMS;       // Start time of application
    static uint64_t lastFrameTimeMS;   // Time of last frame
    static uint64_t gameStartTimeMS;   // Time when it was given Play
};


