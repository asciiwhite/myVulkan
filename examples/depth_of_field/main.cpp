#include "renderer.h"
#include "window.h"

int main(int, char* [])
{
    Window window;
    if (!window.init())
        return -1;

    Renderer renderer;
    if (renderer.init(window.getWindowHandle()))
    {
        window.show();
        window.run(renderer);
    }
    
    renderer.destroy();
    window.destroy();

    return 0;
}
