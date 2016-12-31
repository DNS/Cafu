/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "glfwMainWindow.hpp"
#include "glfwWindow.hpp"
#include "GLFW/glfw3.h"


glfwMainWindowT::glfwMainWindowT(cf::glfwWindowT& win, getGlfwKeyFunc getGlfwKey)
    : MainWindowT(),
      m_Win(win),
      m_getGlfwKey(getGlfwKey)
{
}


unsigned int glfwMainWindowT::GetFrameBufferWidth() const
{
    unsigned int width;
    unsigned int height;

    m_Win.getFramebufferSize(width, height);

    return width;
}


unsigned int glfwMainWindowT::GetFrameBufferHeight() const
{
    unsigned int width;
    unsigned int height;

    m_Win.getFramebufferSize(width, height);

    return height;
}


bool glfwMainWindowT::IsKeyDown(unsigned int Key) const
{
    const int glfwKey = m_getGlfwKey(Key);

    if (glfwKey == 0)
        return false;

    return m_Win.isKeyPressed(glfwKey);
}


bool glfwMainWindowT::LeftMB_IsDown() const
{
    return m_Win.isMouseButtonPressed(GLFW_MOUSE_BUTTON_LEFT);
}


bool glfwMainWindowT::MiddleMB_IsDown() const
{
    return m_Win.isMouseButtonPressed(GLFW_MOUSE_BUTTON_MIDDLE);
}


bool glfwMainWindowT::RightMB_IsDown() const
{
    return m_Win.isMouseButtonPressed(GLFW_MOUSE_BUTTON_RIGHT);
}


void glfwMainWindowT::SwapBuffers()
{
    m_Win.swapBuffers();
}
