#include "simplerenderer.h"

#include <SDL.h>
#include <stdint.h>


int main(int /*argc*/, char* [] /*argv*/)
{
    SDL_Init(SDL_INIT_VIDEO);

    SDL_Window *window = SDL_CreateWindow(
        "myVulkan",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        640,
        480,
        SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN
    );

    SimpleRenderer renderer;
    if (!renderer.init(window))
        return -1;
  
    SDL_ShowWindow(window);

    // main loop
    bool quit = false;
    bool pause = false;
    SDL_Event event;
    while (!quit)
    {
        while (SDL_PollEvent(&event) != 0)
        {
            switch (event.type)
            {
            case SDL_QUIT:
                quit = true;
                break;
            case SDL_WINDOWEVENT:
                switch (event.window.event)
                {
                    case SDL_WINDOWEVENT_SIZE_CHANGED:
                        pause = !renderer.resize(event.window.data1, event.window.data2);
                        break;
                    case SDL_WINDOWEVENT_MINIMIZED:
                        pause = true;
                        break;
                    case SDL_WINDOWEVENT_RESTORED:
                        pause = false;
                        break;
                    case SDL_WINDOWEVENT_CLOSE:
                        //shutdown();
                        break;
                }
                break;
            }         
        }
        if (!pause)
        {
            renderer.draw();
        }
    }

    renderer.destroy();
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}