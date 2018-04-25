#include "simplerenderer.h"
#include "window.h"

#include <GLFW/glfw3.h>

int main(int, char* [])
{
    if (!glfwInit())
        return -1;

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

    glfwTerminate();
    return 0;
}