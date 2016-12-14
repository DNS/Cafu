/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_MAIN_WINDOW_WX_HPP_INCLUDED
#define CAFU_MAIN_WINDOW_WX_HPP_INCLUDED

#include "MainWindow.hpp"

class wxFrame;
class wxGLCanvas;


class MainWindowWxT : public MainWindowT
{
    public:

    /// The constructor.
    MainWindowWxT(wxFrame* Frame, wxGLCanvas* Canvas);

    unsigned int GetFrameBufferWidth() const override;
    unsigned int GetFrameBufferHeight() const override;

    bool IsKeyDown(unsigned int Key) const override;
    bool LeftMB_IsDown() const override;
    bool MiddleMB_IsDown() const override;
    bool RightMB_IsDown() const override;

    void SwapBuffers() override;


    private:

    wxFrame*    m_Frame;
    wxGLCanvas* m_Canvas;
};

#endif
