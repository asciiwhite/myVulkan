#include "simplerenderer.h"
#include "window.h"

int main(int, char* [])
{
    Window window;
    if (!window.init())
        return -1;

    SimpleRenderer renderer;
    if (!renderer.init(window.getWindowHandle()))
        return -1;

    window.show();
    window.run(renderer);
    
    renderer.destroy();
    window.destroy();

    return 0;
}