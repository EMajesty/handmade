#include <SDL2/SDL.h>
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_video.h>
#include <cstdint>
#include <stdlib.h>
#include <sys/mman.h>

#define internal static
#define local_persist static
#define global_variable static

struct sdl_offscreen_buffer {
    SDL_Texture *Texture;
    void *Memory;
    int Width;
    int Height;
    int Pitch;
    int BytesPerPixel;
};

struct sdl_window_dimension {
    int Width;
    int Height;
};

global_variable sdl_offscreen_buffer GlobalBackBuffer;

sdl_window_dimension SDLGetWindowDimension(SDL_Window *Window)
{
    sdl_window_dimension Result;
    SDL_GetWindowSize(Window, &Result.Width, &Result.Height);
    return(Result);
}

internal void RenderWeirdGradient(sdl_offscreen_buffer Buffer, int BlueOffset, int GreenOffset)
{
    uint8_t *Row = (uint8_t *)Buffer.Memory;

    for (int Y = 0; Y < Buffer.Height; ++Y)
    {
        uint32_t *Pixel = (uint32_t *)Row;

        for (int X = 0; X < Buffer.Width; ++X)
        {
            uint8_t Blue = (X + BlueOffset);
            uint8_t Green = (Y + GreenOffset);

            *Pixel++ = ((Green << 8) | Blue);
        }

        Row += Buffer.Pitch;
    }
}

internal void SDLResizeTexture(sdl_offscreen_buffer *Buffer, SDL_Renderer *Renderer, int Width, int Height)
{
    Buffer->BytesPerPixel = 4;

    if (Buffer->Texture)
    {
        SDL_DestroyTexture(Buffer->Texture);
    }
    if (Buffer->Memory)
    {
        // free(BitmapMemory);
        munmap(Buffer->Memory, Buffer->Width * Buffer->Height * Buffer->BytesPerPixel);
    }

    Buffer->Texture = SDL_CreateTexture(Renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, Width, Height);

    Buffer->Width = Width;
    Buffer->Height = Height;

    // BitmapMemory = malloc(Width * Height * BytesPerPixel);
    Buffer->Memory = mmap(0, Width * Height * Buffer->BytesPerPixel, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);

    Buffer->Pitch = Width * Buffer->BytesPerPixel;

    RenderWeirdGradient(GlobalBackBuffer, 0, 0);
}

internal void SDLUpdateWindow(sdl_offscreen_buffer Buffer, SDL_Renderer *Renderer)
{
    SDL_UpdateTexture(Buffer.Texture, 0, Buffer.Memory, Buffer.Width * Buffer.BytesPerPixel);
    SDL_RenderCopy(Renderer, Buffer.Texture, 0, 0);
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
            SDLUpdateWindow(GlobalBackBuffer, Renderer);
        }
        break;
        }
    }
    break;
    }
    return (ShouldQuit);
}

int main(/*int argc, char **argv*/)
{
    SDL_Init(SDL_INIT_VIDEO);

    SDL_Window *Window = SDL_CreateWindow(
        "Hell World", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1920, 1080, SDL_WINDOW_RESIZABLE
    );

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
                RenderWeirdGradient(GlobalBackBuffer, XOffset, YOffset);
                SDLUpdateWindow(GlobalBackBuffer, Renderer);

                ++XOffset;
                YOffset += 2;
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

    SDL_Quit();
    return (0);
}
