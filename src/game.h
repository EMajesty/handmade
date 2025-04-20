#pragma once

#include <cstdint>
#include <math.h>

#define Pi32 3.14159265359f

// Services that the platform layer provides to the game

// Services that the game provides to the platform layer

struct game_sound_output_buffer
{
    int SamplesPerSecond;
    int SampleCount;
    int16_t *Samples;
};

struct game_offscreen_buffer
{
    void *Memory;
    int Width;
    int Height;
    int Pitch;
};

void GameOutputSound(game_sound_output_buffer *SoundBuffer, int ToneHz);
void RenderWeirdGradient(game_offscreen_buffer *OffscreenBuffer, int BlueOffset, int GreenOffset);
void GameUpdateAndRender(
    game_offscreen_buffer *Buffer, int BlueOffset, int GreenOffset, game_sound_output_buffer *SoundBuffer, int ToneHz
);
