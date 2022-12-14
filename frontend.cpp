#include <SDL2/SDL.h>
#include "handler.h"
#include <iostream>
#include <algorithm>

void saveTileMap(Handler& hand) {
    uint8_t vbuf[256*256*4];
    hand.getTileMap(vbuf);
    SDL_Surface *sur =  SDL_CreateRGBSurfaceWithFormatFrom(vbuf, 256, 256, 32, 4*256, SDL_PIXELFORMAT_ARGB8888);
    if (!sur) {
        fprintf(stderr, "could not create surface\n");
        exit(1);
    }

    SDL_SaveBMP(sur, "tilemap.bmp");
}

int main(int argc, char **argv)
{
    Handler handler(!strcmp(argv[1], "debug"));
    handler.init(argv[2]);
    if(SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        std::cout << "Failed to initialize the SDL2 library\n";
        return -1;
    }

    SDL_Window *window = SDL_CreateWindow("SDL2 Window", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 
            4*160, 4*144, SDL_WINDOW_RESIZABLE);

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
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
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
        std::cout << SDL_GetPixelFormatName(info.texture_formats[i]) << std::endl;
    }

    SDL_Texture *texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, 
            SDL_TEXTUREACCESS_STREAMING, 160, 144);
    if (!texture) {
        std::cout << "unsuccessful texture creation\n";
        printf(SDL_GetError());
        return -1;
    }
    uint8_t frameBuffer[160*144*4] = {};
    for (int i=100; i<155; i++){ 
        for (int j=100; j<139; j++) {
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


    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, NULL, NULL);
    SDL_RenderPresent(renderer);

    SDL_Delay(2000);

    Uint64 start_count, delta_count, perf_freq, start_total;
    int delta_t;
    perf_freq = SDL_GetPerformanceFrequency()/1000;
    start_total = SDL_GetPerformanceCounter();
    fprintf(stderr, "before loop, %I64d", SDL_GetPerformanceFrequency());
    //main loop
    bool should_quit = false;
    SDL_Event e;
    Uint64 dots2 = 0;
    while (!should_quit) {
        start_count = SDL_GetPerformanceCounter();

        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT) {
                should_quit = true;
                fprintf(stderr, "quitting");
            }
        }
        while (handler.frame_callback(frameBuffer)) {
            dots2++;
            if (dots2 == 65746996)
                saveTileMap(handler);

            handler.tick();
        }



        //callback
        pitch = 160;
        texture_pixels = nullptr;
        SDL_LockTexture(texture, NULL, &texture_pixels, &pitch);
        memcpy(texture_pixels, frameBuffer, 160*4*144);
        SDL_UnlockTexture(texture);


        //        fprintf(stderr, "before render");

        if (SDL_RenderClear(renderer)) {
            fprintf(stderr, "clear failed\n");
            return -1;
        }
        if (SDL_RenderCopy(renderer, texture, NULL, NULL)) {
            fprintf(stderr, "rendercopy failed\n");
            return -1;
        }
        SDL_RenderPresent(renderer);
        //  fprintf(stderr, "after render\n");

        delta_count = SDL_GetPerformanceCounter() - start_count;
        delta_t = delta_count / perf_freq;
        SDL_Delay(std::max(16-delta_t, 0));
        //fprintf(stderr, "%I64d\n", delta_count);
    }
//    fprintf(stderr, "dots: %I64d, time: %I64d\n", dots2, (SDL_GetPerformanceCounter() - start_total)/perf_freq);
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
