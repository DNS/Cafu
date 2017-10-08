/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_RESOURCES_HPP_INCLUDED
#define CAFU_RESOURCES_HPP_INCLUDED

#ifdef _WIN32
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
#endif

#if __linux__
#define HMODULE void*
#endif

#include <string>


class GameInfoT;
class ClientT;
class ServerT;
class SvGuiCallbT;
class MainWindowT;
class ModelManagerT;
namespace cf { class CompositeConsoleT; }
namespace cf { namespace GuiSys { class GuiResourcesT; } }
namespace cf { namespace GuiSys { class ConsoleByWindowT; } }


class ResourcesT
{
    public:

    /// The constructor. Throws std::runtime_error on failure.
    ResourcesT(cf::CompositeConsoleT& CC, const std::string& ConsoleText, const GameInfoT& GameInfo, MainWindowT& MainWin);

    /// The destructor.
    ~ResourcesT();

    void runFrame(float FrameTimeF);


    private:

    void Cleanup();

    cf::CompositeConsoleT&        m_CompositeConsole;
    HMODULE                       m_RendererDLL;
    ModelManagerT*                m_ModelManager;
    cf::GuiSys::GuiResourcesT*    m_GuiResources;
    HMODULE                       m_SoundSysDLL;
    ClientT*                      m_Client;
    ServerT*                      m_Server;
    SvGuiCallbT*                  m_SvGuiCallback;
    cf::GuiSys::ConsoleByWindowT* m_ConByGuiWin;
};

#endif
