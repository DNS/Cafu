/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "GuiManImpl.hpp"
#include "GuiImpl.hpp"
#include "Window.hpp"
#include "ConsoleCommands/Console.hpp"
#include "TextParser/TextParser.hpp"
#include "MaterialSystem/Renderer.hpp"
#include "Math3D/Matrix.hpp"
#include "OpenGL/OpenGLWindow.hpp"              // Just for the Ca*EventT classes...

#include <assert.h>

using namespace cf::GuiSys;


GuiManImplT::GuiManImplT(GuiResourcesT& GuiRes)
    : m_GuiResources(GuiRes),
      SuppressNextChar(false)
{
    assert(MatSys::Renderer!=NULL);
}


IntrusivePtrT<GuiImplT> GuiManImplT::Register(const std::string& GuiScriptName)
{
    try
    {
        IntrusivePtrT<GuiImplT> Gui = new GuiImplT(m_GuiResources);

        Gui->LoadScript(GuiScriptName);

        Guis.PushBack(Gui);

        return Guis[Guis.Size()-1];
    }
    catch (const GuiImplT::InitErrorT&)
    {
        Console->Print(std::string("GUI \"")+GuiScriptName+"\" not registered due to GuiImplT::InitErrorT exception.\n");
    }

    return NULL;
}


IntrusivePtrT<GuiImplT> GuiManImplT::Register(IntrusivePtrT<GuiImplT> NewGui)
{
    Guis.PushBack(NewGui);

    return NewGui;
}


void GuiManImplT::Free(IntrusivePtrT<GuiImplT> Gui)
{
    for (unsigned long GuiNr=0; GuiNr<Guis.Size(); GuiNr++)
        if (Guis[GuiNr] == Gui)
        {
            Guis.RemoveAtAndKeepOrder(GuiNr);
            return;
        }
}


IntrusivePtrT<GuiImplT> GuiManImplT::Find(const std::string& GuiScriptName, bool AutoRegister)
{
    for (unsigned long GuiNr=0; GuiNr<Guis.Size(); GuiNr++)
        if (Guis[GuiNr]->GetScriptName()==GuiScriptName)
            return Guis[GuiNr];

    return AutoRegister ? Register(GuiScriptName) : NULL;
}


void GuiManImplT::BringToFront(IntrusivePtrT<GuiImplT> Gui)
{
    for (unsigned long GuiNr=0; GuiNr+1<Guis.Size(); GuiNr++)
        if (Guis[GuiNr]==Gui)
        {
            Guis.RemoveAtAndKeepOrder(GuiNr);
            Guis.PushBack(Gui);
            return;
        }
}


IntrusivePtrT<GuiImplT> GuiManImplT::GetTopmostActiveAndInteractive()
{
    for (unsigned long GuiNr=0; GuiNr<Guis.Size(); GuiNr++)
    {
        IntrusivePtrT<GuiImplT> Gui=Guis[Guis.Size()-1-GuiNr];

        if (Gui->GetIsActive() && Gui->GetIsInteractive())
            return Gui;
    }

    return NULL;
}


void GuiManImplT::ReloadAllGuis()
{
    for (unsigned long GuiNr=0; GuiNr<Guis.Size(); GuiNr++)
    {
        if (Guis[GuiNr]->GetScriptName()=="") continue;     // Cannot reload programmatically registered GUIs.

        try
        {
            IntrusivePtrT<GuiImplT> Gui = new GuiImplT(m_GuiResources);

            Gui->LoadScript(Guis[GuiNr]->GetScriptName());

            Guis[GuiNr] = Gui;
        }
        catch (const GuiImplT::InitErrorT&)
        {
            Console->Print(std::string("GUI \"")+Guis[GuiNr]->GetScriptName()+"\" not reloaded due to GuiImplT::InitErrorT exception.\n");
        }
    }
}


void GuiManImplT::RenderAll()
{
    if (Guis.Size()==0) return;

    // First determine the top-most active GUI that fully covers the screen (and thus all the other GUIs below it).
    unsigned long StartGuiNr=Guis.Size()-1;

    while (StartGuiNr>0)
    {
        if (Guis[StartGuiNr]->GetIsActive() && Guis[StartGuiNr]->GetIsFullCover()) break;
        StartGuiNr--;
    }

    // We assume that the caller doesn't expect us to preserve (push/pop) the matrices...
    const float zNear=0.0f;
    const float zFar =1.0f;
    MatSys::Renderer->SetMatrix(MatSys::RendererI::PROJECTION,     MatrixT::GetProjOrthoMatrix(0.0f, VIRTUAL_SCREEN_SIZE_X, VIRTUAL_SCREEN_SIZE_Y, 0.0f, zNear, zFar));
    MatSys::Renderer->SetMatrix(MatSys::RendererI::MODEL_TO_WORLD, MatrixT());
    MatSys::Renderer->SetMatrix(MatSys::RendererI::WORLD_TO_VIEW,  MatrixT());

    // Render the active GUIs in back-to-front order.
    for (unsigned long GuiNr=StartGuiNr; GuiNr<Guis.Size(); GuiNr++)
    {
        if (Guis[GuiNr]->GetIsActive())
            Guis[GuiNr]->Render();
    }
}


void GuiManImplT::ProcessDeviceEvent(const CaKeyboardEventT& KE)
{
    for (unsigned long GuiNr=0; GuiNr<Guis.Size(); GuiNr++)
    {
        IntrusivePtrT<GuiImplT> Gui=Guis[Guis.Size()-1-GuiNr];

        if (Gui->GetIsActive() && Gui->GetIsInteractive())
        {
            switch (KE.Type)
            {
                case CaKeyboardEventT::CKE_KEYDOWN:
                    if (Gui->ProcessDeviceEvent(KE)) SuppressNextChar=true;
                    break;

                case CaKeyboardEventT::CKE_CHAR:
                    if (!SuppressNextChar) Gui->ProcessDeviceEvent(KE);
                    SuppressNextChar=false;
                    break;

                case CaKeyboardEventT::CKE_KEYUP:
                    Gui->ProcessDeviceEvent(KE);
                    SuppressNextChar=false;
                    break;
            }

            break;
        }
    }
}


void GuiManImplT::ProcessDeviceEvent(const CaMouseEventT& ME)
{
    for (unsigned long GuiNr=0; GuiNr<Guis.Size(); GuiNr++)
    {
        IntrusivePtrT<GuiImplT> Gui=Guis[Guis.Size()-1-GuiNr];

        if (Gui->GetIsActive() && Gui->GetIsInteractive())
        {
            Gui->ProcessDeviceEvent(ME);
            break;
        }
    }
}


void GuiManImplT::DistributeClockTickEvents(float t)
{
    for (unsigned long GuiNr=0; GuiNr<Guis.Size(); GuiNr++)
    {
        // Distribute the clock tick events even if Guis[GuiNr]->GetIsActive()==false,
        // because we want to run the pending coroutines of a GUI even if it isn't active.
        Guis[GuiNr]->DistributeClockTickEvents(t);
    }
}
