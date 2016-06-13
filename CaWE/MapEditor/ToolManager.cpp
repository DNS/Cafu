/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "ToolManager.hpp"
#include "Tool.hpp"


ToolManagerT::ToolManagerT(MapDocumentT& MapDoc, wxWindow* ParentToolOptions)
    : m_MapDoc(MapDoc),
      m_Tools(),
      m_ActiveTool(NULL)
{
    const ArrayT<const cf::TypeSys::TypeInfoT*>& Roots=GetToolTIM().GetTypeInfoRoots();

    wxASSERT(Roots.Size()==1);
    if (Roots.Size()==0) return;

    for (const cf::TypeSys::TypeInfoT* TI=Roots[0]; TI; TI=TI->GetNext())
    {
        ToolT* Tool=static_cast<ToolT*>(TI->CreateInstance(ToolT::ToolCreateParamsT(m_MapDoc, *this, ParentToolOptions)));

        // Some tools are not or cannot be created, e.g. the ToolT base class.
        // This way, only tools that the user can actually use enter the m_Tools array.
        if (!Tool) continue;

        m_Tools.PushBack(Tool);
    }
}


ToolManagerT::~ToolManagerT()
{
    for (unsigned long ToolNr=0; ToolNr<m_Tools.Size(); ToolNr++)
        delete m_Tools[ToolNr];
}


ToolT* ToolManagerT::GetTool(const cf::TypeSys::TypeInfoT& Type)
{
    for (unsigned long ToolNr=0; ToolNr<m_Tools.Size(); ToolNr++)
    {
        if (m_Tools[ToolNr]->GetType()==&Type) return m_Tools[ToolNr];
    }

    return NULL;
}


const cf::TypeSys::TypeInfoT* ToolManagerT::GetActiveToolType() const
{
    return m_ActiveTool ? m_ActiveTool->GetType() : NULL;
}


void ToolManagerT::SetActiveTool(const cf::TypeSys::TypeInfoT* Type)
{
    ToolT* NewTool=GetTool(*Type);
    ToolT* OldTool=m_ActiveTool;

    // Cannot set "no tool" as the currently active tool.
    wxASSERT(NewTool);
    if (!NewTool) return;

    if (m_ActiveTool)
    {
        // We currently have an active tool.
        // First test if it can be deactivated - if it cannot, we're done.
        if (!m_ActiveTool->CanDeactivate()) return;

        // Deactive the currently active tool, but only if this isn't just a reactivation of the same tool.
        if (m_ActiveTool!=NewTool) m_ActiveTool->Deactivate(NewTool);
    }

    // Assign the new tool as the currently active tool, and activate it.
    m_ActiveTool=NewTool;
    m_ActiveTool->Activate(OldTool);
}
