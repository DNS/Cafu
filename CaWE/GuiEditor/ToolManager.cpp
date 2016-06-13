/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
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
