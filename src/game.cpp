#include "game.h"

void GameOutputSound(game_sound_output_buffer *SoundBuffer, int ToneHz)
{
    float tSine;
    int16_t ToneVolume = 3000;
    // int ToneHz = 256;
    int WavePeriod = SoundBuffer->SamplesPerSecond / ToneHz;

    int16_t *SampleOut = SoundBuffer->Samples;
    for (int SampleIndex = 0; SampleIndex < SoundBuffer->SampleCount; ++SampleIndex)
    {
        float SineValue = sinf(tSine);
        int16_t SampleValue = (int16_t)(SineValue * ToneVolume);
        *SampleOut++ = SampleValue;
        *SampleOut++ = SampleValue;

        tSine += 2.0f * Pi32 * 1.0f / (float)WavePeriod;
    }
}

void RenderWeirdGradient(game_offscreen_buffer *OffscreenBuffer, int BlueOffset, int GreenOffset)
{
    uint8_t *Row = (uint8_t *)OffscreenBuffer->Memory;

    for (int Y = 0; Y < OffscreenBuffer->Height; ++Y)
    {
        uint32_t *Pixel = (uint32_t *)Row;

        for (int X = 0; X < OffscreenBuffer->Width; ++X)
        {
            uint8_t Blue = (X + BlueOffset);
            uint8_t Green = (Y + GreenOffset);

            *Pixel++ = ((Green << 8) | Blue);
        }

        Row += OffscreenBuffer->Pitch;
    }
}

void GameUpdateAndRender(
    game_offscreen_buffer *OffscreenBuffer, int BlueOffset, int GreenOffset, game_sound_output_buffer *SoundBuffer, int ToneHz
)
{
    // TODO: Allow sample offsets here
    GameOutputSound(SoundBuffer, ToneHz);
    RenderWeirdGradient(OffscreenBuffer, BlueOffset, GreenOffset);
}
