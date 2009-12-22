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

#include "ClientWindow.hpp"
#include "Client.hpp"

#include "GuiSys/WindowCreateParams.hpp"
#include "MaterialSystem/Renderer.hpp"
#include "TypeSys.hpp"

extern "C"
{
    #include <lua.h>
    #include <lualib.h>
    #include <lauxlib.h>
}


/*** Begin of TypeSys related definitions for this class. ***/

void* ClientWindowT::CreateInstance(const cf::TypeSys::CreateParamsT& Params)
{
    return new ClientWindowT(*static_cast<const cf::GuiSys::WindowCreateParamsT*>(&Params));
}

const luaL_reg ClientWindowT::MethodsList[]=
{
    { "__gc", WindowT::Destruct },
    { NULL, NULL }
};

const cf::TypeSys::TypeInfoT ClientWindowT::TypeInfo(cf::GuiSys::GetWindowTIM(), "ClientWindowT", "WindowT", ClientWindowT::CreateInstance, MethodsList);

/*** End of TypeSys related definitions for this class. ***/


ClientWindowT::ClientWindowT(const cf::GuiSys::WindowCreateParamsT& Params)
    : WindowT(Params),
      Client(NULL),
      LastFrameTime(0.0f)
{
    // Setup the window attributes.
    Rect[0]=  0.0f;
    Rect[1]=  0.0f;
    Rect[2]=640.0f;
    Rect[3]=480.0f;
}


void ClientWindowT::SetClient(ClientT* Cl)
{
    Client=Cl;
}


void ClientWindowT::Render() const
{
    // Call the clients render methods here.

    // We don't want want ortho rendering here, so setup a perspective projection matrix.
    // TODO 1: Move this into the world rendering code??
    // TODO 2: Should actually *push* the matrices, properly *set* them and below *pop* them!!
    //         Also see the TODO in GuiManImplT::RenderAll() for more details!!
    MatSys::Renderer->PopMatrix(MatSys::RendererI::PROJECTION    );
    MatSys::Renderer->PopMatrix(MatSys::RendererI::MODEL_TO_WORLD);
    MatSys::Renderer->PopMatrix(MatSys::RendererI::WORLD_TO_VIEW );

    Client->Render(LastFrameTime);

    MatSys::Renderer->PushMatrix(MatSys::RendererI::PROJECTION    );
    MatSys::Renderer->PushMatrix(MatSys::RendererI::MODEL_TO_WORLD);
    MatSys::Renderer->PushMatrix(MatSys::RendererI::WORLD_TO_VIEW );
}


bool ClientWindowT::OnInputEvent(const CaKeyboardEventT& KE)
{
    return Client->ProcessInputEvent(KE);

    // return WindowT::OnInputEvent(KE);    // Ignore the base class.
}


bool ClientWindowT::OnInputEvent(const CaMouseEventT& ME, float /*PosX*/, float /*PosY*/)
{
    return Client->ProcessInputEvent(ME);

    // return WindowT::OnInputEvent(ME);    // Ignore the base class.
}


bool ClientWindowT::OnClockTickEvent(float t)
{
    LastFrameTime=t;

    return WindowT::OnClockTickEvent(t);
}
