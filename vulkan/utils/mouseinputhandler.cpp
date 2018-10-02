#include "camerainputhandler.h"

#include <GLFW/glfw3.h>

MouseInputHandler::MouseInputHandler(CameraInputHandler& handler)
    : m_handler(handler)
{
}

bool MouseInputHandler::update()
{
    if (m_mouseState.buttonIsPressed[0] ||
        m_mouseState.buttonIsPressed[1] ||
        m_mouseState.buttonIsPressed[2])
    {
        m_handler.update(m_mouseState);        
        m_mouseState.lastPosition = m_mouseState.position;        
        return true;
    }

    return false;
}

void MouseInputHandler::button(int button, int action, int mods)
{
    m_handler.setObserverMode((mods & GLFW_MOD_CONTROL) == 0);

    if (button == GLFW_MOUSE_BUTTON_1)
        m_mouseState.buttonIsPressed[0] = action == GLFW_PRESS;
    else if (button == GLFW_MOUSE_BUTTON_3)
        m_mouseState.buttonIsPressed[1] = action == GLFW_PRESS;
    else if (button == GLFW_MOUSE_BUTTON_2)
        m_mouseState.buttonIsPressed[2] = action == GLFW_PRESS;
}

bool MouseInputHandler::move(float x, float y, float deltaTimeInSeconds, int windowWidth, int windowHeight, bool disableHandlerUpdate)
{
    m_mouseState.lastPosition = m_mouseState.position;
    m_mouseState.position[0] = x;
    m_mouseState.position[1] = y;

    if ((m_mouseState.buttonIsPressed[0] ||
        m_mouseState.buttonIsPressed[1] ||
        m_mouseState.buttonIsPressed[2]) &&
        !disableHandlerUpdate)
    {
        m_handler.mouseMove(m_mouseState, deltaTimeInSeconds, windowWidth, windowHeight);
        return true;
    }
    return false;
}

const MouseInputState& MouseInputHandler::getMouseInputState() const
{
    return m_mouseState;
}
