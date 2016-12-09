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


class GameInfoT;
class ClientT;
class ServerT;
class SvGuiCallbT;
class ModelManagerT;
namespace cf { namespace GuiSys { class GuiResourcesT; } }
namespace cf { namespace GuiSys { class ConsoleByWindowT; } }


class ResourcesT
{
    public:

    enum InitStateT { INIT_REQUIRED, INIT_FAILED, INIT_SUCCESS };

    /// The constructor.
    ResourcesT(const GameInfoT& GameInfo);

    /// The destructor.
    ~ResourcesT();

    InitStateT GetInitState() const { return m_InitState; }

    /// Initialize the resources. Throws std::runtime_error on failure.
    void Initialize();


    public:

    void Cleanup();

    const GameInfoT&              m_GameInfo;
    InitStateT                    m_InitState;  ///< Indicates whether initialization is still required, was attempted but failed, or completed successfully.
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
