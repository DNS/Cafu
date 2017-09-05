/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "glfwWindow.hpp"
#include "GLFW/glfw3.h"
#include <stdexcept>

using namespace cf;


static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    glfwWindowT* win = static_cast<glfwWindowT*>(glfwGetWindowUserPointer(window));

    win->KeyEvent(key, scancode, action, mods);
}


static void character_callback(GLFWwindow* window, unsigned int codepoint)
{
    glfwWindowT* win = static_cast<glfwWindowT*>(glfwGetWindowUserPointer(window));

    win->CharEvent(codepoint);
}


static void cursor_pos_callback(GLFWwindow* window, double xpos, double ypos)
{
    glfwWindowT* win = static_cast<glfwWindowT*>(glfwGetWindowUserPointer(window));

    win->MouseMoveEvent(xpos, ypos);
}


static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    glfwWindowT* win = static_cast<glfwWindowT*>(glfwGetWindowUserPointer(window));

    win->MouseButtonEvent(button, action, mods);
}


static void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    glfwWindowT* win = static_cast<glfwWindowT*>(glfwGetWindowUserPointer(window));

    win->MouseScrollEvent(xoffset, yoffset);
}


static void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glfwWindowT* win = static_cast<glfwWindowT*>(glfwGetWindowUserPointer(window));

    win->FramebufferSizeEvent(width, height);
}


glfwWindowT::glfwWindowT(int width, int height, const char* title, GLFWmonitor* monitor)
    : m_win(NULL),
      m_last_fs_width(width),
      m_last_fs_height(height),
      m_last_win_width(width),
      m_last_win_height(height)
{
    printf("Opening %s of size %ix%i.\n", monitor ? "full screen window" : "window", width, height);

    // The default values for the window creations hints look just right for our purposes,
    // see http://www.glfw.org/docs/latest/window_guide.html#window_hints_values for details.
    m_win = glfwCreateWindow(width, height, title, monitor, NULL);

    if (!m_win)
        throw std::runtime_error("glfwCreateWindow() failed.");

    glfwSetWindowUserPointer(m_win, this);
    glfwSetKeyCallback(m_win, key_callback);
    glfwSetCharCallback(m_win, character_callback);
    glfwSetCursorPosCallback(m_win, cursor_pos_callback);
    glfwSetMouseButtonCallback(m_win, mouse_button_callback);
    glfwSetScrollCallback(m_win, scroll_callback);
    glfwSetFramebufferSizeCallback(m_win, framebuffer_size_callback);
}


glfwWindowT::~glfwWindowT()
{
    glfwDestroyWindow(m_win);
}


void glfwWindowT::makeContextCurrent()
{
    glfwMakeContextCurrent(m_win);
}


void glfwWindowT::triggerFramebufferSizeEvent()
{
    int width;
    int height;

    glfwGetFramebufferSize(m_win, &width, &height);
    framebuffer_size_callback(m_win, width, height);
}


void glfwWindowT::getFramebufferSize(unsigned int& width, unsigned int& height) const
{
    glfwGetFramebufferSize(m_win, (int*)&width, (int*)&height);
}


void glfwWindowT::swapBuffers()
{
    glfwSwapBuffers(m_win);
}


bool glfwWindowT::shouldClose() const
{
    return glfwWindowShouldClose(m_win) == GLFW_TRUE;
}


void glfwWindowT::setShouldClose(bool close)
{
    glfwSetWindowShouldClose(m_win, close);
}


void glfwWindowT::setMouseCursorMode(int value)
{
    glfwSetInputMode(m_win, GLFW_CURSOR, value);
}


void glfwWindowT::getMouseCursorPos(double& posX, double& posY) const
{
    glfwGetCursorPos(m_win, &posX, &posY);
}


int glfwWindowT::getWindowAttrib(int attrib) const
{
    return glfwGetWindowAttrib(m_win, attrib);
}


void glfwWindowT::getWindowSize(unsigned int& width, unsigned int& height) const
{
    width = 0;
    height = 0;

    glfwGetWindowSize(m_win, (int*)&width, (int*)&height);
}


void glfwWindowT::toggleFullScreen(bool mode_change)
{
    if (glfwGetWindowMonitor(m_win) == NULL)
    {
        // Switch from windowed to full screen.
        glfwGetWindowSize(m_win, &m_last_win_width, &m_last_win_height);

        GLFWmonitor* monitor = glfwGetPrimaryMonitor();

        if (mode_change)
        {
            printf("Switching to full screen mode %ix%i.\n", m_last_fs_width, m_last_fs_height);
            glfwSetWindowMonitor(m_win, monitor, 0, 0, m_last_fs_width, m_last_fs_height, GLFW_DONT_CARE);
        }
        else
        {
            const GLFWvidmode* mode = glfwGetVideoMode(monitor);  // We're windowed, so this is the desktop mode.

            printf("Switching to desktop full screen mode (%ix%i).\n", mode->width, mode->height);
            glfwSetWindowMonitor(m_win, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
        }
    }
    else
    {
        // Switch from full screen to windowed.
        int fs_width;
        int fs_height;

        glfwGetWindowSize(m_win, &fs_width, &fs_height);

        printf("Switching to window of size %ix%i.\n", m_last_win_width, m_last_win_height);
        glfwSetWindowMonitor(m_win, NULL, 320, 240, m_last_win_width, m_last_win_height, GLFW_DONT_CARE);

        GLFWmonitor* monitor = glfwGetPrimaryMonitor();
        const GLFWvidmode* mode = glfwGetVideoMode(monitor);  // We're windowed now, so this is the desktop mode.

        if (fs_width != mode->width || fs_height != mode->height)
        {
            // The previous full screen mode was not the desktop full screen mode.
            m_last_fs_width  = fs_width;
            m_last_fs_height = fs_height;
        }
    }
}


bool glfwWindowT::isKeyPressed(int key) const
{
    return glfwGetKey(m_win, key) == GLFW_PRESS;
}


bool glfwWindowT::isMouseButtonPressed(int button) const
{
    return glfwGetMouseButton(m_win, button) == GLFW_PRESS;
}
