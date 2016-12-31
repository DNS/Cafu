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
    : m_win(NULL)
{
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
    glfwGetWindowSize(m_win, (int*)&width, (int*)&height);
}


bool glfwWindowT::isKeyPressed(int key) const
{
    return glfwGetKey(m_win, key) == GLFW_PRESS;
}


bool glfwWindowT::isMouseButtonPressed(int button) const
{
    return glfwGetMouseButton(m_win, button) == GLFW_PRESS;
}
