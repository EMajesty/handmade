#include <SDL2/SDL.h>
#include <SDL2/SDL_render.h>
#include <stdlib.h>
#include <sys/mman.h>

#define internal static
#define local_persist static
#define global_variable static

global_variable SDL_Texture *Texture;
global_variable void *BitmapMemory;
global_variable int BitmapWidth;
global_variable int BytesPerPixel = 4;

internal void SDLResizeTexture(SDL_Renderer *Renderer, int Width, int Height)
{
    if (Texture)
    {
        SDL_DestroyTexture(Texture);
    }
    if (BitmapMemory)
    {
        // free(BitmapMemory);
        munmap(BitmapMemory, Width * Height * BytesPerPixel);
    }

    Texture = SDL_CreateTexture(Renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, Width, Height);

    // BitmapMemory = malloc(Width * Height * BytesPerPixel);
    BitmapMemory = mmap(0, Width * Height * BytesPerPixel, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
}

internal void SDLUpdateWindow(SDL_Window *Window, SDL_Renderer *Renderer)
{
    if (SDL_UpdateTexture(Texture, 0, BitmapMemory, BitmapWidth * BytesPerPixel))
    {
        // TODO:
    }
    SDL_RenderCopy(Renderer, Texture, 0, 0);
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
            SDL_Window *Window = SDL_GetWindowFromID(Event->window.windowID);
            SDL_Renderer *Renderer = SDL_GetRenderer(Window);
            SDLResizeTexture(Renderer, Event->window.data1, Event->window.data2);
            printf("SDL_WINDOWEVENT_SIZE_CHANGED (%d, %d)\n", Event->window.data1, Event->window.data2);
        }
        break;
        case SDL_WINDOWEVENT_EXPOSED:
        {
            SDL_Window *Window = SDL_GetWindowFromID(Event->window.windowID);
            SDL_Renderer *Renderer = SDL_GetRenderer(Window);
            SDLUpdateWindow(Window, Renderer);
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
    if (SDL_Init(SDL_INIT_VIDEO) != 0)
    {
    }
    SDL_Window *Window = SDL_CreateWindow(
        "Hell World", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 640, 480, SDL_WINDOW_RESIZABLE
    );

    if (Window)
    {
        SDL_Renderer *Renderer = SDL_CreateRenderer(Window, -1, 0);

        if (Renderer)
        {
            for (;;)
            {
                SDL_Event Event;
                SDL_WaitEvent(&Event);
                if (HandleEvent(&Event))
                {
                    break;
                }
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
