#pragma once

#include "glm.h"

struct MouseInputState
{
    bool buttonIsPressed[3] = { false };
    glm::vec2 position = glm::vec2(0.0);
    glm::vec2 lastPosition = glm::vec2(0.0);
};

class CameraInputHandler;

class MouseInputHandler
{
public:
    MouseInputHandler(CameraInputHandler&);

    void button(int button, int action, int mods);
    bool move(float x, float y, float deltaTimeInSeconds, int windowWidth, int windowHeight, bool disableHandlerUpdate);
    bool update();

    const MouseInputState& getMouseInputState() const;

private:
    MouseInputState m_mouseState;

    CameraInputHandler& m_handler;
};