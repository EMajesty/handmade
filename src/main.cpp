#include <SDL2/SDL.h>
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_gamecontroller.h>
#include <SDL2/SDL_haptic.h>
#include <SDL2/SDL_joystick.h>
#include <SDL2/SDL_keycode.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_video.h>
#include <cstdint>
#include <cstdio>
#include <stdlib.h>
#include <sys/mman.h>

#define internal static
#define local_persist static
#define global_variable static

#define MAX_CONTROLLERS 4

struct sdl_offscreen_buffer
{
    // Pixels are always 32 bits wide, memory order BB GG RR XX
    SDL_Texture *Texture;
    void *Memory;
    int Width;
    int Height;
    int Pitch;
    // int BytesPerPixel;
};

struct sdl_window_dimension
{
    int Width;
    int Height;
};

global_variable sdl_offscreen_buffer GlobalBackBuffer;

sdl_window_dimension SDLGetWindowDimension(SDL_Window *Window)
{
    sdl_window_dimension Result;
    SDL_GetWindowSize(Window, &Result.Width, &Result.Height);
    return (Result);
}

internal void RenderWeirdGradient(sdl_offscreen_buffer *Buffer, int BlueOffset, int GreenOffset)
{
    uint8_t *Row = (uint8_t *)Buffer->Memory;

    for (int Y = 0; Y < Buffer->Height; ++Y)
    {
        uint32_t *Pixel = (uint32_t *)Row;

        for (int X = 0; X < Buffer->Width; ++X)
        {
            uint8_t Blue = (X + BlueOffset);
            uint8_t Green = (Y + GreenOffset);

            *Pixel++ = ((Green << 8) | Blue);
        }

        Row += Buffer->Pitch;
    }
}

internal void SDLResizeTexture(sdl_offscreen_buffer *Buffer, SDL_Renderer *Renderer, int Width, int Height)
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

    RenderWeirdGradient(&GlobalBackBuffer, 0, 0);
}

internal void SDLUpdateWindow(sdl_offscreen_buffer *Buffer, SDL_Renderer *Renderer)
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
            // SDLResizeTexture(&GlobalBackBuffer, Renderer, Event->window.data1, Event->window.data2);
        }
        break;
        case SDL_WINDOWEVENT_EXPOSED:
        {
            SDL_Window *Window = SDL_GetWindowFromID(Event->window.windowID);
            SDL_Renderer *Renderer = SDL_GetRenderer(Window);
            SDLUpdateWindow(&GlobalBackBuffer, Renderer);
        }
        break;
        }
    }
    break;

    case SDL_KEYDOWN:
    case SDL_KEYUP:
    {
        // SDL_Keycode KeyCode = Event->key.keysym.sym;

        if (!Event->key.repeat)
        {
            printf("%d %d\n", Event->key.keysym.sym, Event->key.state);
        }
    }
    break;
    }
    return (ShouldQuit);
}

int main(/*int argc, char **argv*/)
{
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER | SDL_INIT_HAPTIC);

    SDL_Window *Window = SDL_CreateWindow(
        "Hell World", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1920, 1080, SDL_WINDOW_RESIZABLE
    );

    // Open controllers
    SDL_GameController *ControllerHandles[MAX_CONTROLLERS];
    SDL_Haptic *RumbleHandles[MAX_CONTROLLERS];

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

    if (Window)
    {
        SDL_Renderer *Renderer = SDL_CreateRenderer(Window, -1, 0);

        if (Renderer)
        {
            bool Running = true;
            // int Width, Height;
            sdl_window_dimension Dimension = SDLGetWindowDimension(Window);
            SDLResizeTexture(&GlobalBackBuffer, Renderer, Dimension.Width, Dimension.Height);
            int XOffset = 0;
            int YOffset = 0;

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

                // TODO: Poll this more often?
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

                        XOffset += LeftStickX >> 12;
                        YOffset += LeftStickY >> 12;
                    }
                    else
                    {
                        // TODO: This controller is not plugged in
                    }
                }

                RenderWeirdGradient(&GlobalBackBuffer, XOffset, YOffset);
                SDLUpdateWindow(&GlobalBackBuffer, Renderer);

                // ++XOffset;
                // YOffset += 2;
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

    // Close haptics and controllers
    for (int ControllerIndex = 0; ControllerIndex < MAX_CONTROLLERS; ++ControllerIndex)
    {
        if (RumbleHandles[ControllerIndex])
        {
            SDL_HapticClose(RumbleHandles[ControllerIndex]);
        }

        if (ControllerHandles[ControllerIndex])
        {
            SDL_GameControllerClose(ControllerHandles[ControllerIndex]);
        }
    }

    SDL_Quit();
    return (0);
}
