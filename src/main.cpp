#include <iostream>
#include <SDL2/SDL.h>

int main(int argc, char **argv) {
    std::cout << "Hell world" << std::endl;
    
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, "Hell World", "Hell World", 0);
    return(0);
}
