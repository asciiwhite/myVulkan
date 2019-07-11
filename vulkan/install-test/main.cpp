#include "window.h"

int main(int, char* [])
{
    Window window;
    if (!window.init())
        return -1;
 
    window.show();
    window.destroy();

    return 0;
}
