/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_MAIN_WINDOW_HPP_INCLUDED
#define CAFU_MAIN_WINDOW_HPP_INCLUDED


class MainWindowT
{
    public:

    /// The constructor.
    MainWindowT(class MainFrameT* MainFrame);

    /// The destructor.
    ~MainWindowT();

    unsigned int GetFrameBufferWidth() const;
    unsigned int GetFrameBufferHeight() const;

    bool IsKeyDown(unsigned int Key) const;
    bool LeftMB_IsDown() const;
    bool MiddleMB_IsDown() const;
    bool RightMB_IsDown() const;

    void SwapBuffers();


    private:

    MainFrameT* m_MainFrame;
};

#endif
