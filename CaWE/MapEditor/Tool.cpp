/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
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
