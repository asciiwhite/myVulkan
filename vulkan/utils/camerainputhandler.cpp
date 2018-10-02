#include "camerainputhandler.h"
#include "arcball_camera.h"
#include "flythrough_camera.h"

#include <algorithm>

void CameraInputHandler::setObserverMode(bool observerMode)
{
    m_observerCameraMode = observerMode;
}

void CameraInputHandler::setCameraFromBoundingBox(const glm::vec3& min, const glm::vec3& max, const glm::vec3& lookDir)
{
    const auto size = max - min;
    const auto center = (min + max) / 2.f;
    m_sceneBoundingBoxDiameter = std::max(std::max(size.x, size.y), size.z);
    const auto cameraDistance = m_sceneBoundingBoxDiameter * 1.5f;

    m_cameraPosition = glm::vec3(cameraDistance) * glm::normalize(lookDir) - center;
    m_cameraTarget = center;
    m_cameraLook = glm::normalize(m_cameraTarget - m_cameraPosition);
    m_cameraUp = glm::vec3(0, -1, 0);
}

glm::mat4 CameraInputHandler::mvp(float aspectRatio) const
{
    const auto lookVec = m_observerCameraMode ? m_cameraTarget : m_cameraPosition + m_cameraLook;

    const glm::mat4 view = glm::lookAt(m_cameraPosition, lookVec, m_cameraUp);
    const glm::mat4 projection = glm::perspective(glm::radians(45.0f), aspectRatio, 0.01f, 20.f * m_sceneBoundingBoxDiameter);
    const glm::mat4 mvp = projection * view;

    return mvp;
}

void CameraInputHandler::update(const MouseInputState& m_mouseState)
{
    if (m_observerCameraMode)
        return;

    const auto stepSize = 0.05f * m_sceneBoundingBoxDiameter;

    const auto deltaX = static_cast<int>(m_mouseState.position[0] - m_mouseState.lastPosition[0]);
    const auto deltaY = static_cast<int>(m_mouseState.position[1] - m_mouseState.lastPosition[1]);

    flythrough_camera_update(
        glm::value_ptr(m_cameraPosition),
        glm::value_ptr(m_cameraLook),
        glm::value_ptr(m_cameraUp),
        nullptr,
        0.01f,
        stepSize,
        0.5f,
        80.0f,
        deltaX, deltaY,
        m_mouseState.buttonIsPressed[2],
        m_mouseState.buttonIsPressed[1],
        0,
        0,
        0, 0, 0);

    m_cameraTarget += m_cameraLook;
}

void CameraInputHandler::mouseMove(const MouseInputState& m_mouseState, float deltaTimeInSeconds, int windowWidth, int windowHeight)
{
    if (!m_observerCameraMode)
        return;

    const auto zoomPerTicks = 0.005f * m_sceneBoundingBoxDiameter;
    const auto panSpeed = 7.0f * m_sceneBoundingBoxDiameter;
    const auto rotationMultiplier = 5.0f;

    arcball_camera_update(
        glm::value_ptr(m_cameraPosition),
        glm::value_ptr(m_cameraTarget),
        glm::value_ptr(m_cameraUp),
        nullptr,
        deltaTimeInSeconds,
        zoomPerTicks, panSpeed, rotationMultiplier,
        windowWidth, windowHeight,
        static_cast<int>(m_mouseState.lastPosition[0]), static_cast<int>(m_mouseState.position[0]),
        static_cast<int>(m_mouseState.position[1]), static_cast<int>(m_mouseState.lastPosition[1]),  // inverse axis for correct rotation direction
        m_mouseState.buttonIsPressed[2],
        m_mouseState.buttonIsPressed[0],
        m_mouseState.buttonIsPressed[1] ? static_cast<int>(m_mouseState.position[1] - m_mouseState.lastPosition[1]) : 0,
        0);

    m_cameraLook = glm::normalize(m_cameraTarget - m_cameraPosition);
}