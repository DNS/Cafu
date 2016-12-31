/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_MAIN_WINDOW_GLFW_HPP_INCLUDED
#define CAFU_MAIN_WINDOW_GLFW_HPP_INCLUDED

#include "MainWindow.hpp"


namespace cf { class glfwWindowT; }


class glfwMainWindowT : public MainWindowT
{
    public:

    typedef int (*getGlfwKeyFunc)(int);

    /// The constructor.
    glfwMainWindowT(cf::glfwWindowT& win, getGlfwKeyFunc getGlfwKey);

    unsigned int GetFrameBufferWidth() const override;
    unsigned int GetFrameBufferHeight() const override;

    bool IsKeyDown(unsigned int Key) const override;
    bool LeftMB_IsDown() const override;
    bool MiddleMB_IsDown() const override;
    bool RightMB_IsDown() const override;

    void SwapBuffers() override;


    private:

    cf::glfwWindowT& m_Win;
    getGlfwKeyFunc   m_getGlfwKey;
};

#endif
