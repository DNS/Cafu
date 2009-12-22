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

#include "ToolManager.hpp"

#include "ToolSelection.hpp"


using namespace GuiEditor;


ToolManagerT::ToolManagerT(GuiDocumentT* GuiDocument, ChildFrameT* Parent)
    : m_ActiveTool(NULL)
{
    // Create tools for the GUI document managed by the the tool manager.
    m_Tools.PushBack(new ToolSelectionT(GuiDocument, Parent));

    SetActiveTool(TOOL_SELECTION);
}


ToolManagerT::~ToolManagerT()
{
    if (m_ActiveTool)
    {
        m_ActiveTool->Deactivate();
        m_ActiveTool=NULL;
    }

    for (unsigned long ToolNr=0; ToolNr<m_Tools.Size(); ToolNr++)
        delete m_Tools[ToolNr];
}


ToolI* ToolManagerT::GetActiveTool()
{
    return m_ActiveTool;
}


void ToolManagerT::SetActiveTool(ToolID ToolID)
{
    // Deactivate currently active tool if any.
    if (m_ActiveTool)
    {
        m_ActiveTool->Deactivate();
        m_ActiveTool=NULL;
    }

    for (unsigned long ToolNr=0; ToolNr<m_Tools.Size(); ToolNr++)
    {
        if (m_Tools[ToolNr]->GetID()==ToolID)
        {
            m_ActiveTool=m_Tools[ToolNr];
            m_ActiveTool->Activate();
        }
    }
}
