/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_CLIENT_MAIN_WINDOW_HPP_INCLUDED
#define CAFU_CLIENT_MAIN_WINDOW_HPP_INCLUDED

#include "MainWindow/glfwWindow.hpp"
#include "Util/Util.hpp"
#include "Resources.hpp"


namespace cf { class CompositeConsoleT; }
class GameInfoT;


/// This class represents the OS's main window for the Cafu client.
class ClientMainWindowT : public cf::glfwWindowT
{
    public:

    /// The constructor.
    ClientMainWindowT(const GameInfoT& GameInfo, int width, int height, const char* title, GLFWmonitor* monitor=0);

    /// The destructor.
    ~ClientMainWindowT();

    void Init(cf::CompositeConsoleT& CC, const std::string& ConsoleText, MainWindowT& MainWin);
    void runFrame();

    void FramebufferSizeEvent(int width, int height) override;
    void KeyEvent(int key, int scancode, int action, int mods) override;
    void CharEvent(unsigned int codepoint) override;
    void MouseMoveEvent(double xpos, double ypos) override;
    void MouseButtonEvent(int button, int action, int mods) override;
    void MouseScrollEvent(double xoffset, double yoffset) override;

    static int getCaKey(int glfwKey);
    static int getGlfwKey(int caKey);


    private:

    enum LastMousePosT { IN_CLIENT_3D_GUI, IN_OTHER_2D_GUI };

    void TakeScreenshot() const;

    const GameInfoT& m_GameInfo;
    ResourcesT*      m_Resources;
    TimerT           m_Timer;
    double           m_TotalTime;
    LastMousePosT    m_LastMousePos;   ///< Used to prevent unwanted changes to the player's heading and pitch when we're switching back from a 2D GUI to the 3D client view.
    double           m_LastMousePosX;
    double           m_LastMousePosY;
};

#endif
