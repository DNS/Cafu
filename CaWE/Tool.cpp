/*
=================================================================================
This file is part of Cafu, the open-source game engine and graphics engine
for multiplayer, cross-platform, real-time 3D action.
Copyright (C) 2002-2012 Carsten Fuchs Software.

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

#include "Tool.hpp"
#include "ChildFrameViewWin2D.hpp"
#include "ChildFrameViewWin3D.hpp"
#include "ToolManager.hpp"
#include "MapDocument.hpp"


// Note that we cannot simply replace this method with a global TypeInfoManT instance,
// because it is called during global static initialization time. The TIM instance being
// embedded in the function guarantees that it is properly initialized before first use.
cf::TypeSys::TypeInfoManT& GetToolTIM()
{
    static cf::TypeSys::TypeInfoManT TIM;

    return TIM;
}


ToolT::ToolCreateParamsT::ToolCreateParamsT(MapDocumentT& MapDoc_, ToolManagerT& ToolMan_, wxWindow* ParentOptionsBar_)
    : MapDoc(MapDoc_),
      ToolMan(ToolMan_),
      ParentOptionsBar(ParentOptionsBar_)
{
}


/*** Begin of TypeSys related definitions for this class. ***/

void* ToolT::CreateInstance(const cf::TypeSys::CreateParamsT& Params)
{
    return NULL;
}

const cf::TypeSys::TypeInfoT ToolT::TypeInfo(GetToolTIM(), "ToolT", NULL, ToolT::CreateInstance, NULL);

/*** End of TypeSys related definitions for this class. ***/


ToolT::ToolT(MapDocumentT& MapDoc, ToolManagerT& ToolMan)
    : m_MapDoc(MapDoc),
      m_ToolMan(ToolMan),
      m_IsActiveTool(false)
{
}


void ToolT::Activate(ToolT* OldTool)
{
    OnActivate(OldTool);
    m_IsActiveTool=true;

    m_ToolMan.UpdateAllObservers(this, UPDATE_SOON);
}


void ToolT::Deactivate(ToolT* NewTool)
{
    OnDeactivate(NewTool);
    m_IsActiveTool=false;
}


int ToolT::OnContextMenu2D(ViewWindow2DT& ViewWindow, wxContextMenuEvent& CE, wxMenu& Menu)
{
    return ViewWindow.GetPopupMenuSelectionFromUser(Menu);
}


int ToolT::OnContextMenu3D(ViewWindow3DT& ViewWindow, wxContextMenuEvent& CE, wxMenu& Menu)
{
    return ViewWindow.GetPopupMenuSelectionFromUser(Menu);
}
