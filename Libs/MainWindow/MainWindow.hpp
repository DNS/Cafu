/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_MAIN_WINDOW_HPP_INCLUDED
#define CAFU_MAIN_WINDOW_HPP_INCLUDED


/// This class represents the OS's application window, abstracting it so that
/// the Cafu Engine is independent of it's concrete implementation.
/// Possible implementations include GLFW, wxWidgets or DirectX (or our former
/// SingleOpenGLWindowT).
class MainWindowT
{
    public:

    /// The constructor.
    MainWindowT() { }

    /// The virtual destructor.
    virtual ~MainWindowT() { }

    virtual unsigned int GetFrameBufferWidth() const = 0;
    virtual unsigned int GetFrameBufferHeight() const = 0;

    virtual bool IsKeyDown(unsigned int Key) const = 0;
    virtual bool LeftMB_IsDown() const = 0;
    virtual bool MiddleMB_IsDown() const = 0;
    virtual bool RightMB_IsDown() const = 0;

    virtual void SwapBuffers() = 0;


    private:

    MainWindowT(const MainWindowT&);        ///< Use of the Copy    Constructor is not allowed.
    void operator = (const MainWindowT&);   ///< Use of the Assignment Operator is not allowed.
};

#endif
