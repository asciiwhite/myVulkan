#pragma once

#include "mouseinputhandler.h"
#include "glm.h"

class CameraInputHandler
{
public:
    void setCameraFromBoundingBox(const glm::vec3& min, const glm::vec3& max, const glm::vec3& lookDir);
    void setObserverMode(bool observerMode);

    void mouseMove(const MouseInputState& mouseState, float deltaTimeInSeconds, int windowWidth, int windowHeight);
    void update(const MouseInputState& mouseState);

    glm::mat4x4 mvp(float aspectRatio) const;
    
private:
    glm::vec3 m_cameraPosition = glm::vec3(0.0);
    glm::vec3 m_cameraTarget = glm::vec3(0.0);
    glm::vec3 m_cameraLook = glm::vec3(0.0);
    glm::vec3 m_cameraUp = glm::vec3(0.0);

    float m_sceneBoundingBoxDiameter = 0.f;
    bool m_observerCameraMode = true;
};
