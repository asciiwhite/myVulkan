#include "window.h"
#include "basicrenderer.h"

#include <GLFW/glfw3.h>
#include <iostream>

static void glfwErrorCallback(int error, const char* description)
{
    std::cout << "glfw error #" << error << " : " << description << "\n";
}

bool Window::init()
{
    glfwSetErrorCallback(glfwErrorCallback);

    if (!glfwInit())
        return false;

    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    m_window = glfwCreateWindow(640, 480, "myVulkan", nullptr, nullptr);
    if (m_window)
    {
        glfwSetWindowUserPointer(m_window, this);
        glfwSetWindowSizeCallback(m_window, [](GLFWwindow* window, int w, int h) {
            static_cast<Window*>(glfwGetWindowUserPointer(window))->resize(w, h);
        });        
        glfwSetMouseButtonCallback(m_window, [](GLFWwindow* window, int button, int action, int mods) {
            static_cast<Window*>(glfwGetWindowUserPointer(window))->mouseButton(button, action, mods);
        });
        glfwSetCursorPosCallback(m_window, [](GLFWwindow* window, double x, double y) {
            static_cast<Window*>(glfwGetWindowUserPointer(window))->mouseMove(x, y);
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
    glfwTerminate();
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

void Window::mouseButton(int button, int action, int mods)
{
    m_renderer->mouseButton(button, action, mods);
}

void Window::mouseMove(double x, double y)
{
    m_renderer->mouseMove(x, y);
}

GLFWwindow* Window::getWindowHandle() const
{
    return m_window;
}

