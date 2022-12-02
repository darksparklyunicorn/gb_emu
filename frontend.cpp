#include <SDL2/SDL.hi>
#include "handler.h"
#include <iostream>

int main(int argc, char **argv)
{
    Handler handler;
    if(SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        std::cout << "Failed to initialize the SDL2 library\n";
        return -1;
    }

    SDL_Window *window = SDL_CreateWindow("SDL2 Window", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 
            160, 144, SDL_WINDOW_RESIZABLE);

    if(!window)
    {
        std::cout << "Failed to create window\n";
        return -1;
    }
    /*    
          SDL_Surface *window_surface = SDL_GetWindowSurface(window);

          if(!window_surface)
          {
          std::cout << "Failed to get the surface from the window\n";
          return -1;
          }
          */
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) { 
        std::cout << "failed to create renderer\n";
        return -1;
    }

    if (SDL_RenderSetLogicalSize(renderer, 160, 144)) {
        std::cout << "failed to set renderer size\n";
        return -1;
    }
    SDL_RendererInfo info;
    if (SDL_GetRendererInfo(renderer, &info)) {
        std::cout << "failed to get info\n";
        return -1;
    }
    for (Uint32 i = 0; i<info.num_texture_formats; i++) {
        //    std::cout << SDL_GetPixelFormatName(info.texture_formats[i]) << std::endl;
    }

    SDL_Texture *texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, 
            SDL_TEXTUREACCESS_STREAMING, 160, 144);
    if (!texture) {
        std::cout << "unsuccessful texture creation\n";
        printf(SDL_GetError());
        return -1;
    }
    uint8_t frameBuffer[160*144*4] = {};
    for (int i=0; i<20; i++){ 
        for (int j=0; j<20; j++) {
            frameBuffer[(i+j*160)*4] = 0xff;
            frameBuffer[(i+j*160)*4+1] = 0xff;
            frameBuffer[(i+j*160)*4+2] = 0xff;
            frameBuffer[(i+j*160)*4+3] = 0xff;
        }
    }
    //callback
    int pitch = 160;
    void *texture_pixels = nullptr;
    SDL_LockTexture(texture, NULL, &texture_pixels, &pitch);
    memcpy(texture_pixels, frameBuffer, 160*4*144);
    SDL_UnlockTexture(texture);

    //render
    bool should_quit = false;
    SDL_Event e;
    while (!should_quit) {
        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT) {
                should_quit = true;
            }
        }
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, texture, NULL, NULL);
        SDL_RenderPresent(renderer);
    }

    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
