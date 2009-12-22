/*
=================================================================================
This file is part of Cafu, the open-source game and graphics engine for
multiplayer, cross-platform, real-time 3D action.
$Id$

Copyright (C) 2002-2010 Carsten Fuchs Software.

Cafu is free software: you can redistribute it and/or modify it under the terms
of the GNU General Public License as published by the Free Software Foundation,
either version 3 of the License, or (at your option) any later version.

Cafu is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Cafu. If not, see <http://www.gnu.org/licenses/>.

For support and more information about Cafu, visit us at <http://www.cafu.de>.
=================================================================================
*/

#include "GuiManImpl.hpp"
#include "Gui.hpp"
#include "GuiImpl.hpp"
#include "InitWindowTypes.hpp"
#include "ConsoleCommands/Console.hpp"
#include "TextParser/TextParser.hpp"
#include "MaterialSystem/Renderer.hpp"          // For dealing with the default RM.
#include "MaterialSystem/MaterialManager.hpp"   // For dealing with the default RM.
#include "Math3D/Matrix.hpp"
#include "Fonts/FontTT.hpp"                     // For dealing with the default font.
#include "OpenGL/OpenGLWindow.hpp"              // Just for the Ca*EventT classes...

#include <assert.h>

using namespace cf::GuiSys;


static const unsigned long InitDummy=InitWindowTypes();


GuiManImplT::GuiManImplT()
    : GuiDefaultRM(NULL),
      SuppressNextChar(false)
{
    assert(MatSys::Renderer!=NULL);

    GuiDefaultRM=MatSys::Renderer->RegisterMaterial(MaterialManager->GetMaterial("Gui/Default"));
}


GuiManImplT::~GuiManImplT()
{
    // Delete all the GUIs.
    for (unsigned long GuiNr=0; GuiNr<Guis.Size(); GuiNr++)
        delete Guis[GuiNr];

    // Free the Fonts.
    for (unsigned long FontNr=0; FontNr<Fonts.Size(); FontNr++)
        delete Fonts[FontNr];

    // Free the GuiDefaultRM.
    MatSys::Renderer->FreeMaterial(GuiDefaultRM);
}


GuiI* GuiManImplT::Register(const std::string& GuiScriptName)
{
    try
    {
        Guis.PushBack(new GuiImplT(GuiScriptName));

        return Guis[Guis.Size()-1];
    }
    catch (const GuiImplT::InitErrorT&)
    {
        Console->Print(std::string("GUI \"")+GuiScriptName+"\" not registered due to GuiImplT::InitErrorT exception.\n");
    }

    return NULL;
}


GuiI* GuiManImplT::Register(GuiI* NewGui)
{
    Guis.PushBack(NewGui);

    return NewGui;
}


void GuiManImplT::Free(GuiI* Gui)
{
    for (unsigned long GuiNr=0; GuiNr<Guis.Size(); GuiNr++)
        if (Guis[GuiNr]==Gui)
        {
            delete Gui;
            Guis.RemoveAtAndKeepOrder(GuiNr);
            return;
        }
}


GuiI* GuiManImplT::Find(const std::string& GuiScriptName, bool AutoRegister)
{
    for (unsigned long GuiNr=0; GuiNr<Guis.Size(); GuiNr++)
        if (Guis[GuiNr]->GetScriptName()==GuiScriptName)
            return Guis[GuiNr];

    return AutoRegister ? Register(GuiScriptName) : NULL;
}


void GuiManImplT::BringToFront(GuiI* Gui)
{
    for (unsigned long GuiNr=0; GuiNr+1<Guis.Size(); GuiNr++)
        if (Guis[GuiNr]==Gui)
        {
            Guis.RemoveAtAndKeepOrder(GuiNr);
            Guis.PushBack(Gui);
            return;
        }
}


GuiI* GuiManImplT::GetTopmostActiveAndInteractive()
{
    for (unsigned long GuiNr=0; GuiNr<Guis.Size(); GuiNr++)
    {
        GuiI* Gui=Guis[Guis.Size()-1-GuiNr];

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
            GuiI* Reloaded=new GuiImplT(Guis[GuiNr]->GetScriptName());

            delete Guis[GuiNr];
            Guis[GuiNr]=Reloaded;
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

    // Render the GUIs in back-to-front order.
    for (unsigned long GuiNr=StartGuiNr; GuiNr<Guis.Size(); GuiNr++)
    {
        // TODO:
        // 1) Remove the next (no-comment-) line.
        // 2) Move the matrix push, set and pop code out of this loop!
        // 3) Fix the clients ClientWindowT::Render() method by fixing the matrix setup code inside it
        //    (it probably belongs into the world rendering method anyway)!!
        if (!Guis[GuiNr]->GetIsActive()) continue;

        // Setup the matrices.
        MatSys::Renderer->PushMatrix(MatSys::RendererI::PROJECTION    );
        MatSys::Renderer->PushMatrix(MatSys::RendererI::MODEL_TO_WORLD);
        MatSys::Renderer->PushMatrix(MatSys::RendererI::WORLD_TO_VIEW );

        const float zNear=0.0f;
        const float zFar =1.0f;
        MatSys::Renderer->SetMatrix(MatSys::RendererI::PROJECTION,     MatrixT::GetProjOrthoMatrix(0.0f, VIRTUAL_SCREEN_SIZE_X, VIRTUAL_SCREEN_SIZE_Y, 0.0f, zNear, zFar));
        MatSys::Renderer->SetMatrix(MatSys::RendererI::MODEL_TO_WORLD, MatrixT());
        MatSys::Renderer->SetMatrix(MatSys::RendererI::WORLD_TO_VIEW,  MatrixT());

        if (Guis[GuiNr]->GetIsActive()) Guis[GuiNr]->Render();

        // Restore the previously active matrices.
        MatSys::Renderer->PopMatrix(MatSys::RendererI::PROJECTION    );
        MatSys::Renderer->PopMatrix(MatSys::RendererI::MODEL_TO_WORLD);
        MatSys::Renderer->PopMatrix(MatSys::RendererI::WORLD_TO_VIEW );
    }
}


void GuiManImplT::ProcessDeviceEvent(const CaKeyboardEventT& KE)
{
    for (unsigned long GuiNr=0; GuiNr<Guis.Size(); GuiNr++)
    {
        GuiI* Gui=Guis[Guis.Size()-1-GuiNr];

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
        GuiI* Gui=Guis[Guis.Size()-1-GuiNr];

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


MatSys::RenderMaterialT* GuiManImplT::GetDefaultRM() const
{
    return GuiDefaultRM;
}


cf::TrueTypeFontT* GuiManImplT::GetFont(const std::string& FontName)
{
    // See if FontName has been loaded successfully before.
    for (unsigned long FontNr=0; FontNr<Fonts.Size(); FontNr++)
        if (Fonts[FontNr]->GetName()==FontName)
            return Fonts[FontNr];

    // See if FontName has been loaded UNsuccessfully before.
 // for (unsigned long FontNr=0; FontNr<FontsFailed.Size(); FontNr++)
 //     if (FontsFailed[FontNr]==FontName)
 //         return NULL;

    // FontName has never been attempted to be loaded, try now.
    try
    {
        Fonts.PushBack(new cf::TrueTypeFontT(FontName));
        return Fonts[Fonts.Size()-1];
    }
    catch (const TextParserT::ParseError&) { }

    Console->Warning(std::string("Failed to load font \"")+FontName+"\".\n");
 // FontsFailed.PushBack(FontName);
    return Fonts.Size()>0 ? Fonts[0] : NULL;
}
