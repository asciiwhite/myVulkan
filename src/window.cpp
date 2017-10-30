#include "window.h"
#include "vulkan/basicrenderer.h"

#include <glfw/glfw3.h>

bool Window::init()
{
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    m_window = glfwCreateWindow(640, 480, "myVulkan", NULL, NULL);
    if (m_window)
    {
        glfwSetWindowUserPointer(m_window, this);
        glfwSetWindowSizeCallback(m_window, [](GLFWwindow* window, int w, int h) {
            static_cast<Window*>(glfwGetWindowUserPointer(window))->resize(w, h);
        });
        return true;
    }

    return false;
}

void Window::destroy()
{
    if (m_window)
    {
        glfwDestroyWindow(m_window);
        m_window = nullptr;
    }
}

void Window::show()
{
    glfwShowWindow(m_window);
}

void Window::run(BasicRenderer& renderer)
{
    m_renderer = &renderer;
    while (!glfwWindowShouldClose(m_window))
    {
        if (!m_pause)
        {
            renderer.update();
            renderer.draw();
        }

        glfwPollEvents();
    }
}

void Window::resize(int width, int height)
{
    m_pause = (width * height == 0);
    if (m_renderer && !m_pause)
        m_renderer->resize(width, height);
}

GLFWwindow* Window::getWindowHandle() const
{
    return m_window;
}

