/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "glfwMainWindow.hpp"
#include "glfwWindow.hpp"
#include "GLFW/glfw3.h"


glfwMainWindowT::glfwMainWindowT(cf::glfwWindowT& win)
    : MainWindowT(),
      m_Win(win)
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
    // Must translate the CaKeyboardEventT::CK_* key to a wxWidgets key.
    // TODO: int wxKey = translateCKToWx(Key);
    // return wxGetKeyState(wxKey);
    return false;
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
