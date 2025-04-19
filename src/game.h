#pragma once

struct game_offscreen_buffer
{
    void *Memory;
    int Width;
    int Height;
    int Pitch;
};

void GameUpdateAndRender(game_offscreen_buffer *Buffer, int BlueOffset, int GreenOffset);
void RenderWeirdGradient(game_offscreen_buffer *Buffer, int BlueOffset, int GreenOffset);

