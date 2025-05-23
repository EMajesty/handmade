#include <SDL2/SDL.h>
#include <cstdint>
#include <sys/mman.h>
#include <x86intrin.h>

#include "game.h"

#define MAX_CONTROLLERS 4

struct sdl_sound_output
{
    int SamplesPerSecond;
    int ToneHz;
    int16_t ToneVolume;
    uint32_t RunningSampleIndex;
    int WavePeriod;
    int BytesPerSample;
    float_t tSine;
    int LatencySampleCount;
};

// struct sdl_audio_ring_buffer
// {
//     int Size;
//     int WriteCursor;
//     int PlayCursor;
//     void *Data;
// };

struct sdl_window_dimension
{
    int Width;
    int Height;
};

struct sdl_offscreen_buffer
{
    SDL_Texture *Texture;
    void *Memory;
    int Width;
    int Height;
    int Pitch;
};

// aka global variable
// static sdl_sound_output SoundOutput;
// static sdl_audio_ring_buffer AudioRingBuffer;
static sdl_offscreen_buffer SDLOffscreenBuffer;

SDL_GameController *ControllerHandles[MAX_CONTROLLERS];
SDL_Haptic *RumbleHandles[MAX_CONTROLLERS];

// aka internal function /// no export tables
// static void SDLFillSoundBuffer(sdl_sound_output *SoundOutput, int BytesToWrite)
// {
//     void *SoundBuffer = malloc(BytesToWrite);
//     int16_t *SampleOut = (int16_t *)SoundBuffer;
//     int SampleCount = BytesToWrite / SoundOutput->BytesPerSample;
//     for (int SampleIndex = 0; SampleIndex < SampleCount; ++SampleIndex)
//     {
//         float_t SineValue = sinf(SoundOutput->tSine);
//         int16_t SampleValue = (int16_t)(SineValue * SoundOutput->ToneVolume);
//         *SampleOut++ = SampleValue;
//         *SampleOut++ = SampleValue;
//
//         SoundOutput->tSine += 2.0f * Pi32 * 1.0f / (float)SoundOutput->WavePeriod;
//         ++SoundOutput->RunningSampleIndex;
//     }
//
//     SDL_QueueAudio(1, SoundBuffer, BytesToWrite);
//     free(SoundBuffer);
// }

static void SDLFillSoundBuffer(
    sdl_sound_output *SoundOutput, int ByteToLock, int BytesToWrite, game_sound_output_buffer *SoundBuffer
)
{
    SDL_QueueAudio(1, SoundBuffer->Samples, BytesToWrite);
}

static void SDLInitAudio(int32_t SamplesPerSecond, int32_t BufferSize)
{
    SDL_AudioSpec AudioSettings = {};

    AudioSettings.freq = SamplesPerSecond;
    AudioSettings.format = AUDIO_S16LSB;
    AudioSettings.channels = 2;
    AudioSettings.samples = BufferSize;

    SDL_OpenAudio(&AudioSettings, 0);
    // SDL_OpenAudioDevice(0, 0, &AudioSettings, 0, 0);

    if (AudioSettings.format != AUDIO_S16LSB)
    {
        printf("Can't set audio format to AUDIO_S16LSB\n");
        SDL_CloseAudio();
    }
}

static void SDLInitGameControllers()
{
    int MaxJoysticks = SDL_NumJoysticks();
    int ControllerIndex = 0;
    for (int JoystickIndex = 0; JoystickIndex < MaxJoysticks; ++JoystickIndex)
    {
        if (!SDL_IsGameController(JoystickIndex))
        {
            continue;
        }
        if (ControllerIndex >= MAX_CONTROLLERS)
        {
            break;
        }
        ControllerHandles[ControllerIndex] = SDL_GameControllerOpen(JoystickIndex);

        // Rumble
        SDL_Joystick *JoystickHandle = SDL_GameControllerGetJoystick(ControllerHandles[ControllerIndex]);
        RumbleHandles[ControllerIndex] = SDL_HapticOpenFromJoystick(JoystickHandle);

        if (SDL_HapticRumbleInit(RumbleHandles[ControllerIndex]) != 0)
        {
            printf("No rumble support for controller %d\n", ControllerIndex);
            SDL_HapticClose(RumbleHandles[ControllerIndex]);
            RumbleHandles[ControllerIndex] = 0;
        }
        else
        {
            printf("Rumble enabled for controller %d\n", ControllerIndex);
        }

        ControllerIndex++;
    }
}

static void SDLCloseGameControllers()
{
    for (int ControllerIndex = 0; ControllerIndex < MAX_CONTROLLERS; ++ControllerIndex)
    {
        if (ControllerHandles[ControllerIndex])
        {
            if (RumbleHandles[ControllerIndex])
            {
                SDL_HapticClose(RumbleHandles[ControllerIndex]);
            }

            SDL_GameControllerClose(ControllerHandles[ControllerIndex]);
        }
    }
}

sdl_window_dimension SDLGetWindowDimension(SDL_Window *Window)
{
    sdl_window_dimension Result;
    SDL_GetWindowSize(Window, &Result.Width, &Result.Height);
    return Result;
}

static void SDLResizeTexture(sdl_offscreen_buffer *Buffer, SDL_Renderer *Renderer, int Width, int Height)
{
    const int BytesPerPixel = 4;

    if (Buffer->Texture)
    {
        SDL_DestroyTexture(Buffer->Texture);
    }
    if (Buffer->Memory)
    {
        munmap(Buffer->Memory, Buffer->Width * Buffer->Height * BytesPerPixel);
    }

    Buffer->Texture = SDL_CreateTexture(Renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, Width, Height);

    Buffer->Width = Width;
    Buffer->Height = Height;

    Buffer->Memory =
        mmap(0, Width * Height * BytesPerPixel, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);

    Buffer->Pitch = Width * BytesPerPixel;
}

static void SDLUpdateWindow(sdl_offscreen_buffer *Buffer, SDL_Renderer *Renderer)
{
    SDL_UpdateTexture(Buffer->Texture, 0, Buffer->Memory, Buffer->Pitch);
    SDL_RenderCopy(Renderer, Buffer->Texture, 0, 0);
    SDL_RenderPresent(Renderer);
}

bool HandleEvent(SDL_Event *Event)
{
    bool ShouldQuit = false;

    switch (Event->type)
    {
    case SDL_QUIT:
    {
        printf("SDL_QUIT\n");
        ShouldQuit = true;
    }
    break;

    case SDL_WINDOWEVENT:
    {
        switch (Event->window.event)
        {
        case SDL_WINDOWEVENT_SIZE_CHANGED:
        {
            // SDL_Window *Window = SDL_GetWindowFromID(Event->window.windowID);
            // SDL_Renderer *Renderer = SDL_GetRenderer(Window);
            // printf("SDL_WINDOWEVENT_SIZE_CHANGED (%d, %d)\n", Event->window.data1, Event->window.data2);
            // SDLResizeTexture(&SDLOffscreenBuffer, Renderer, Event->window.data1, Event->window.data2);
        }
        break;
        case SDL_WINDOWEVENT_EXPOSED:
        {
            SDL_Window *Window = SDL_GetWindowFromID(Event->window.windowID);
            SDL_Renderer *Renderer = SDL_GetRenderer(Window);
            SDLUpdateWindow(&SDLOffscreenBuffer, Renderer);
        }
        break;
        }
    }
    break;

    case SDL_KEYDOWN:
    case SDL_KEYUP:
    {
        SDL_Keycode KeyCode = Event->key.keysym.sym;
        bool AltKeyDown = (Event->key.keysym.mod & KMOD_ALT);
        if (KeyCode == SDLK_F4 && AltKeyDown)
        {
            ShouldQuit = true;
        }

        if (!Event->key.repeat)
        {
            if (KeyCode == SDLK_SPACE && Event->key.state == 1)
            {
                // SoundOutput.ToneHz = 512;
                // SoundOutput.WavePeriod = SoundOutput.SamplesPerSecond / SoundOutput.ToneHz;
            }
            else if (KeyCode == SDLK_SPACE && Event->key.state == 0)
            {
                // SoundOutput.ToneHz = 256;
                // SoundOutput.WavePeriod = SoundOutput.SamplesPerSecond / SoundOutput.ToneHz;
            }
            printf("%d %d\n", Event->key.keysym.sym, Event->key.state);
        }
    }
    break;
    }
    return ShouldQuit;
}

int main(/*int argc, char **argv*/)
{
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMECONTROLLER | SDL_INIT_HAPTIC);

    SDL_Window *Window = SDL_CreateWindow(
        "Hell World", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1920, 1080, SDL_WINDOW_RESIZABLE
    );

    SDLInitGameControllers();

    uint64_t PerfCountFrequency = SDL_GetPerformanceFrequency();

    if (Window)
    {
        SDL_Renderer *Renderer = SDL_CreateRenderer(Window, -1, 0);

        if (Renderer)
        {
            bool Running = true;
            sdl_window_dimension Dimension = SDLGetWindowDimension(Window);
            SDLResizeTexture(&SDLOffscreenBuffer, Renderer, Dimension.Width, Dimension.Height);
            int XOffset = 0;
            int YOffset = 0;

            sdl_sound_output SoundOutput = {};

            SoundOutput.SamplesPerSecond = 48000;
            SoundOutput.ToneHz = 256;
            SoundOutput.ToneVolume = 3000;
            SoundOutput.RunningSampleIndex = 0;
            SoundOutput.WavePeriod = SoundOutput.SamplesPerSecond / SoundOutput.ToneHz;
            SoundOutput.BytesPerSample = sizeof(int16_t) * 2;
            SoundOutput.LatencySampleCount = SoundOutput.SamplesPerSecond / 15;

            SDLInitAudio(SoundOutput.SamplesPerSecond, SoundOutput.SamplesPerSecond * SoundOutput.BytesPerSample / 60);

            int16_t *Samples = (int16_t *)calloc(SoundOutput.LatencySampleCount, SoundOutput.BytesPerSample);
            SDL_PauseAudio(0);

            uint64_t LastCounter = SDL_GetPerformanceCounter();
            uint64_t LastCycleCount = _rdtsc();

            while (Running)
            {
                SDL_Event Event;
                // SDL_WaitEvent(&Event);
                while (SDL_PollEvent(&Event))
                {
                    if (HandleEvent(&Event))
                    {
                        Running = false;
                    }
                }

                for (int ControllerIndex = 0; ControllerIndex < MAX_CONTROLLERS; ++ControllerIndex)
                {
                    if (SDL_GameControllerGetAttached(ControllerHandles[ControllerIndex]))
                    {
                        bool Up = SDL_GameControllerGetButton(
                            ControllerHandles[ControllerIndex], SDL_CONTROLLER_BUTTON_DPAD_UP
                        );
                        bool Down = SDL_GameControllerGetButton(
                            ControllerHandles[ControllerIndex], SDL_CONTROLLER_BUTTON_DPAD_DOWN
                        );
                        bool Left = SDL_GameControllerGetButton(
                            ControllerHandles[ControllerIndex], SDL_CONTROLLER_BUTTON_DPAD_LEFT
                        );
                        bool Right = SDL_GameControllerGetButton(
                            ControllerHandles[ControllerIndex], SDL_CONTROLLER_BUTTON_DPAD_RIGHT
                        );
                        bool Start = SDL_GameControllerGetButton(
                            ControllerHandles[ControllerIndex], SDL_CONTROLLER_BUTTON_START
                        );
                        bool Back =
                            SDL_GameControllerGetButton(ControllerHandles[ControllerIndex], SDL_CONTROLLER_BUTTON_BACK);
                        bool LeftShoulder = SDL_GameControllerGetButton(
                            ControllerHandles[ControllerIndex], SDL_CONTROLLER_BUTTON_LEFTSHOULDER
                        );
                        bool RightShoulder = SDL_GameControllerGetButton(
                            ControllerHandles[ControllerIndex], SDL_CONTROLLER_BUTTON_RIGHTSHOULDER
                        );
                        bool AButton =
                            SDL_GameControllerGetButton(ControllerHandles[ControllerIndex], SDL_CONTROLLER_BUTTON_A);
                        bool BButton =
                            SDL_GameControllerGetButton(ControllerHandles[ControllerIndex], SDL_CONTROLLER_BUTTON_B);
                        bool XButton =
                            SDL_GameControllerGetButton(ControllerHandles[ControllerIndex], SDL_CONTROLLER_BUTTON_X);
                        bool YButton =
                            SDL_GameControllerGetButton(ControllerHandles[ControllerIndex], SDL_CONTROLLER_BUTTON_Y);

                        int16_t LeftTrigger = SDL_GameControllerGetAxis(
                            ControllerHandles[ControllerIndex], SDL_CONTROLLER_AXIS_TRIGGERLEFT
                        );
                        int16_t RightTrigger = SDL_GameControllerGetAxis(
                            ControllerHandles[ControllerIndex], SDL_CONTROLLER_AXIS_TRIGGERRIGHT
                        );

                        int16_t LeftStickX =
                            SDL_GameControllerGetAxis(ControllerHandles[ControllerIndex], SDL_CONTROLLER_AXIS_LEFTX);
                        int16_t LeftStickY =
                            SDL_GameControllerGetAxis(ControllerHandles[ControllerIndex], SDL_CONTROLLER_AXIS_LEFTY);
                        int16_t RightStickX =
                            SDL_GameControllerGetAxis(ControllerHandles[ControllerIndex], SDL_CONTROLLER_AXIS_RIGHTX);
                        int16_t RightStickY =
                            SDL_GameControllerGetAxis(ControllerHandles[ControllerIndex], SDL_CONTROLLER_AXIS_RIGHTY);

                        if (Up)
                        {
                            YOffset -= 4;
                        }
                        else if (Down)
                        {
                            YOffset += 4;
                        }
                        if (Left)
                        {
                            XOffset -= 4;
                        }
                        else if (Right)
                        {
                            XOffset += 4;
                        }

                        XOffset += LeftStickX / 4096;
                        YOffset += LeftStickY / 4096;

                        printf("%d %d\n", LeftStickX, LeftStickY);
                    }
                    else
                    {
                        // TODO: This controller is not plugged in
                    }
                }

                int TargetQueueBytes = SoundOutput.LatencySampleCount * SoundOutput.BytesPerSample;
                int BytesToWrite = TargetQueueBytes - SDL_GetQueuedAudioSize(1);
                game_sound_output_buffer SoundBuffer = {};
                SoundBuffer.SamplesPerSecond = SoundOutput.SamplesPerSecond;
                SoundBuffer.SampleCount = BytesToWrite / SoundOutput.BytesPerSample;
                SoundBuffer.Samples = Samples;

                game_offscreen_buffer GameOffscreenBuffer = {};
                GameOffscreenBuffer.Memory = SDLOffscreenBuffer.Memory;
                GameOffscreenBuffer.Width = SDLOffscreenBuffer.Width;
                GameOffscreenBuffer.Height = SDLOffscreenBuffer.Height;
                GameOffscreenBuffer.Pitch = SDLOffscreenBuffer.Pitch;

                GameUpdateAndRender(&GameOffscreenBuffer, XOffset, YOffset, &SoundBuffer, SoundOutput.ToneHz);

                SDLFillSoundBuffer(&SoundOutput, 0, BytesToWrite, &SoundBuffer);

                SDLUpdateWindow(&SDLOffscreenBuffer, Renderer);

                uint64_t EndCounter = SDL_GetPerformanceCounter();
                uint64_t CounterElapsed = EndCounter - LastCounter;

                double MSPerFrame = (((1000.0f * (double)CounterElapsed) / (double)PerfCountFrequency));
                double FPS = (double)PerfCountFrequency / (double)CounterElapsed;

                uint64_t EndCycleCount = _rdtsc();
                uint64_t CyclesElapsed = EndCycleCount - LastCycleCount;

                double MCPF = ((double)CyclesElapsed / (1000.0f * 1000.0f));

                printf("%.02f ms/f, %.02f f/s, %.02f mc/f\n", MSPerFrame, FPS, MCPF);

                LastCounter = EndCounter;
                LastCycleCount = EndCycleCount;
            }
        }
        else
        {
            printf("No renderer");
        }
    }
    else
    {
        printf("No window");
    }

    SDLCloseGameControllers();

    SDL_CloseAudio();

    SDL_Quit();
    return 0;
}
