/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "DialogEntityInspector.hpp"
#include "ChildFrame.hpp"
#include "CompMapEntity.hpp"
#include "MapDocument.hpp"
#include "MapEntRepres.hpp"

#include "../VarVisitors.hpp"

#include "Commands/AddComponent.hpp"
#include "Commands/DeleteComponent.hpp"

#include "wx/artprov.h"
#include "wx/notifmsg.h"


using namespace MapEditor;


/*
 * The two classes
 *
 *     MapEditor::EntityInspectorDialogT
 *     GuiEditor::WindowInspectorT
 *
 * are "siblings" of each other, and the have lots of code in common.
 * Maybe we can merge their code in the future?
 * (Possibly combining other code as well, e.g. having a common ComponentBaseT class?)
 */


BEGIN_EVENT_TABLE(EntityInspectorDialogT, wxPropertyGridManager)
    EVT_PG_CHANGING(wxID_ANY, EntityInspectorDialogT::OnPropertyGridChanging)
    EVT_PG_CHANGED(wxID_ANY, EntityInspectorDialogT::OnPropertyGridChanged)
    EVT_PG_RIGHT_CLICK(wxID_ANY, EntityInspectorDialogT::OnPropertyGridRightClick)
END_EVENT_TABLE()


EntityInspectorDialogT::EntityInspectorDialogT(wxWindow* Parent, ChildFrameT* ChildFrame, const wxSize& Size)
    : wxPropertyGridManager(Parent, wxID_ANY, wxDefaultPosition, Size, wxPG_BOLD_MODIFIED | wxPG_SPLITTER_AUTO_CENTER | wxPG_DESCRIPTION),
      m_MapDocument(ChildFrame->GetDoc()),
      m_ChildFrame(ChildFrame),
      m_SelectedEntity(NULL),
      m_IsRecursiveSelfNotify(false)
{
    // Using wxPG_EX_HELP_AS_TOOLTIPS is an alternative to wxPG_DESCRIPTION above,
    // but tooltips seemed to be too intrusive and nervous for permanent use...
    // Moreover, tooltips don't seem to work with wxPropertyCategory properties.
    // SetExtraStyle(wxPG_EX_HELP_AS_TOOLTIPS);

    AddPage("Entity Properties");

    m_MapDocument->RegisterObserver(this);
    RefreshPropGrid();
}


EntityInspectorDialogT::~EntityInspectorDialogT()
{
    if (m_MapDocument) m_MapDocument->UnregisterObserver(this);
}


void EntityInspectorDialogT::NotifySubjectChanged_Selection(SubjectT* Subject, const ArrayT<MapElementT*>& OldSelection, const ArrayT<MapElementT*>& NewSelection)
{
    // TODO: Only do this if there is at least one EntRepresT among the Old and New selection?
    RefreshPropGrid();
}


void EntityInspectorDialogT::NotifySubjectChanged_Deleted(SubjectT* Subject, const ArrayT< IntrusivePtrT<cf::GameSys::EntityT> >& Entities)
{
    if (m_SelectedEntity.IsNull()) return;

    // Check if our currently selected entity is one of the deleted entities.
    for (unsigned long EntNr = 0; EntNr < Entities.Size(); EntNr++)
    {
        if (m_SelectedEntity == Entities[EntNr])
        {
            m_SelectedEntity = NULL;
            ClearPage(0);
            RefreshGrid();
            break;
        }
    }
}


void EntityInspectorDialogT::NotifySubjectChanged_Modified(SubjectT* Subject, const ArrayT<MapElementT*>& MapElements, MapElemModDetailE Detail)
{
    if (m_IsRecursiveSelfNotify) return;
    if (m_SelectedEntity.IsNull()) return;
    if (MapElements.Find(GetMapEnt(m_SelectedEntity)->GetRepres()) == -1) return;

    switch (Detail)
    {
        case MEMD_GENERIC:       // Intentional fall-through.
        case MEMD_TRANSFORM:
        case MEMD_VISIBILITY:
        {
            RefreshPropGrid();
            break;
        }

        case MEMD_PRIMITIVE_PROPS_CHANGED:  // Intentional fall-through.
        case MEMD_SURFACE_INFO_CHANGED:
        case MEMD_ASSIGN_PRIM_TO_ENTITY:
        {
            // Neither of these affects the Entity Inspector dialog.
            break;
        }
    }
}


void EntityInspectorDialogT::NotifySubjectChanged_Modified(SubjectT* Subject, const ArrayT<MapElementT*>& MapElements, MapElemModDetailE Detail, const ArrayT<BoundingBox3fT>& OldBounds)
{
    NotifySubjectChanged_Modified(Subject, MapElements, Detail);
}


void EntityInspectorDialogT::Notify_EntChanged(SubjectT* Subject, const ArrayT< IntrusivePtrT<MapEditor::CompMapEntityT> >& Entities, EntityModDetailE Detail)
{
    if (m_IsRecursiveSelfNotify) return;
    if (m_SelectedEntity.IsNull()) return;
    if (Entities.Find(GetMapEnt(m_SelectedEntity)) == -1) return;

    switch (Detail)
    {
        case EMD_COMPONENTS:
        {
            RefreshPropGrid();
            break;
        }

        case EMD_HIERARCHY:
        {
            // Changes to the entity hierarchy will usually keep the world-space transfrom constant, and thus
            // update the entity's parent-space origin and orientation accordingly, so we have to update the
            // property grid to reflect these changes.
            RefreshPropGrid();
            break;
        }
    }
}


namespace
{
    /// Recursively updates all children of Prop, except for the one whose variable is SelfNotify.
    void RecUpdateChildren(wxPGProperty* Prop, const cf::TypeSys::VarBaseT* SelfNotify)
    {
        for (unsigned int i = 0; i < Prop->GetChildCount(); i++)
        {
            wxPGProperty*          Child = Prop->Item(i);
            cf::TypeSys::VarBaseT* Var   = static_cast<cf::TypeSys::VarBaseT*>(Child->GetClientData());

            if (Var && Var != SelfNotify)
            {
                VarVisitorUpdatePropT UpdateProp(*Child);

                Var->accept(UpdateProp);
            }

            RecUpdateChildren(Child, SelfNotify);
        }
    }
}


void EntityInspectorDialogT::Notify_VarChanged(SubjectT* Subject, const cf::TypeSys::VarBaseT& Var)
{
    // if (m_IsRecursiveSelfNotify) return;   // Not here, see below.
    if (!GetPage(0)) return;

    // Find the property that is related to Var.
    for (wxPropertyGridIterator it = GetPage(0)->GetIterator(); !it.AtEnd(); it++)
    {
        wxPGProperty* Prop = *it;

        if (Prop->GetClientData() == &Var)
        {
            // Found the proper property for Var, but the straightforward update
            //
            //     if (m_IsRecursiveSelfNotify) return;
            //     VarVisitorUpdatePropT UpdateProp(*Prop);
            //
            //     Var.accept(UpdateProp);
            //     RefreshGrid();
            //     break;
            //
            // is unfortunately not enough: Setting a new value for Var may have caused, as a side-effect, changes
            // to other VarBaseT variables in the same component ("siblings") as well. Therefore, we first find the
            // component (parent) property of Prop, then update all variables / properties in the component:
            while (Prop && !Prop->IsCategory())
                Prop = Prop->GetParent();

            wxASSERT(Prop && Prop->IsCategory());
            if (!Prop || !Prop->IsCategory()) return;

            RecUpdateChildren(Prop, m_IsRecursiveSelfNotify ? &Var : NULL);
            RefreshGrid();
            break;
        }
    }
}


void EntityInspectorDialogT::NotifySubjectDies(SubjectT* dyingSubject)
{
    wxASSERT(dyingSubject == m_MapDocument);

    m_SelectedEntity = NULL;
    m_MapDocument = NULL;

    ClearPage(0);
}


void EntityInspectorDialogT::AppendComponent(IntrusivePtrT<cf::GameSys::ComponentBaseT> Comp)
{
    const ArrayT<cf::TypeSys::VarBaseT*>& MemberVars = Comp->GetMemberVars().GetArray();
    const wxString                        UniqueName = wxString::Format("%p", Comp.get());
    wxPGProperty*                         CatProp    = new wxPropertyCategory(Comp->GetName(), UniqueName);

    // With "category" properties we set the component pointer as client data,
    // with other properties we set (in the AddProp visitor) the variable as client data.
    CatProp->SetClientData(Comp.get());
    CatProp->SetHelpString(Comp->GetType()->DocClass ? Comp->GetType()->DocClass : "");
    Append(CatProp);

    VarVisitorAddPropT AddProp(*this, m_MapDocument->GetAdapter(), Comp->GetType());

    for (unsigned long VarNr = 0; VarNr < MemberVars.Size(); VarNr++)
        MemberVars[VarNr]->accept(AddProp);
}


void EntityInspectorDialogT::RefreshPropGrid()
{
    if (m_MapDocument == NULL) return;

    ClearPage(0);

    ArrayT<MapElementT*> Selection = m_MapDocument->GetSelection();

    // Remove all entities (and primitives) whose parents are in the selection as well.
    MapDocumentT::Reduce(Selection);

    // Remove all remaining primitives from the Selection, keep only the entities.
    for (unsigned int SelNr = 0; SelNr < Selection.Size(); SelNr++)
    {
        if (Selection[SelNr]->GetType() != &MapEntRepresT::TypeInfo)
        {
            Selection.RemoveAt(SelNr);
            SelNr--;
        }
    }

    if (Selection.Size() == 1)
    {
        m_SelectedEntity = Selection[0]->GetParent()->GetEntity();

        AppendComponent(m_SelectedEntity->GetBasics());
        AppendComponent(m_SelectedEntity->GetTransform());

        for (unsigned long CompNr = 0; CompNr < m_SelectedEntity->GetComponents().Size(); CompNr++)
            AppendComponent(m_SelectedEntity->GetComponents()[CompNr]);
    }
    else
    {
        wxString InfoMessage = "Nothing selected";

        if (Selection.Size() > 1) InfoMessage = "Multiple selection";

        // Multiple selection and no selection are handled by showing an info message in the property grid.
        wxPGProperty* Info = Append(new wxStringProperty("Info", wxPG_LABEL, InfoMessage));
        DisableProperty(Info);

        m_SelectedEntity = NULL;
    }

    RefreshGrid();
}


/*
 * Normally, if we make a change to e.g. a "double" property and press RETURN, we see events as expected:
 *
 *     a1) OnPropertyGridChanging: property "RotAngle": double("45")
 *     a2) OnPropertyGridChanged:  property "RotAngle": double("45")
 *
 * Unfortunately, when the property is "<composed>", the sequence of events (with wxWidgets 2.9.2) is not so clear:
 * If "Pos" is a "<composed>" property with "x", "y" and "z" as sub-properties, changing "y" yields:
 *
 *     b1) OnPropertyGridChanging: property "y": double("120")
 *     b2) OnPropertyGridChanged:  property "y": double("120")
 *     b3) OnPropertyGridChanged:  property "Pos": string("0; 120; 200")
 *
 * Note that there is no "changing" event for Pos!
 * If instead we directly change the "z" sub-property in the top-level "Pos" string:
 *
 *     c1) OnPropertyGridChanging: property "Pos": string("0; 120; 240")
 *     c2) OnPropertyGridChanged:  property "Pos": string("0; 120; 240")
 *
 * Note that there is no event at all related to the changed "z" value!
 */
void EntityInspectorDialogT::OnPropertyGridChanging(wxPropertyGridEvent& Event)
{
    // Changing a property by pressing ENTER doesn't change the selection. In consequence the property refresh below does not result in
    // any change since selected properties are not updated (because the user could be in the process of editing a value).
    // Since the user is definitely finished editing this property we can safely clear the selection.
    // ClearSelection();
    wxASSERT(Event.CanVeto());  // EVT_PG_CHANGING events can be vetoed (as opposed to EVT_PG_CHANGED events).
    wxLogDebug("%s: property \"%s\": %s(\"%s\")", __FUNCTION__, Event.GetProperty()->GetLabel(), Event.GetValue().GetType(), Event.GetValue().MakeString());

    const wxPGProperty*    Prop  = Event.GetProperty();
    unsigned int           Depth = 0;
    cf::TypeSys::VarBaseT* Var   = NULL;

    // Note that cases a1) and c1) break the loop at Depth == 0
    // whereas case b1) breaks the loop at Depth == 1.
    while (Prop && !Prop->IsCategory() && Depth < 5)
    {
        Var = static_cast<cf::TypeSys::VarBaseT*>(Prop->GetClientData());
        if (Var) break;
        Prop = Prop->GetParent();
        Depth++;
    }

    wxLogDebug("    ==> Var == \"%s\" at Prop \"%s\" (%u above Prop \"%s\")",
        Var ? Var->GetName() : "NULL", Prop ? Prop->GetLabel(): "NULL", Depth, Event.GetProperty()->GetLabel());
    if (!Var) return;

    m_IsRecursiveSelfNotify = true;

    VarVisitorHandlePropChangingEventT PropChange(Event, Depth, m_MapDocument->GetAdapter());
    Var->accept(PropChange);
    if (!m_ChildFrame->SubmitCommand(PropChange.TransferCommand())) Event.Veto();

    if (!Event.WasVetoed() && Var && Var->GetExtraMessage() != "")
    {
        // A wxInfoBar is a possible alternative to wxNotificationMessage.
        wxNotificationMessage Notify(wxString("Setting \"") + Var->GetName() + "\"", Var->GetExtraMessage(), m_ChildFrame);

        Notify.Show();
    }

    m_IsRecursiveSelfNotify = false;
}


void EntityInspectorDialogT::OnPropertyGridChanged(wxPropertyGridEvent& Event)
{
    // Changing a property by pressing ENTER doesn't change the selection. In consequence the property refresh below does not result in
    // any change since selected properties are not updated (because the user could be in the process of editing a value).
    // Since the user is definitely finished editing this property we can safely clear the selection.
    // ClearSelection();
    wxLogDebug("%s: property \"%s\": %s(\"%s\")", __FUNCTION__, Event.GetProperty()->GetLabel(), Event.GetValue().GetType(), Event.GetValue().MakeString());

    // In OnPropertyGridChanging(), we essentially and eventually call code like
    //
    //     Var.Set(Event.GetValue());    // pseudo-code
    //
    // Note that if Var is of a type derived from cf::TypeSys::VarT<>, the Set() method may well set
    // Var to a value that is different from Event.GetValue().
    // For example, if the variable contains a filename, the Set() method may flip backslashes into
    // forward slashes, make filenames relative, clamp numeric values to min-max range, turn arbitrary
    // strings into valid Lua identifiers, etc.
    // Unfortunately, there seems to be no way to pass back the actual value of Var back into the
    // EVT_PG_CHANGING event processing directly in OnPropertyGridChanging() (at least not in wx-2.9.2),
    // but it *is* possible here, so we take the opportunity.
    wxPGProperty*          Prop = Event.GetProperty();
    cf::TypeSys::VarBaseT* Var  = Prop && !Prop->IsCategory() ? static_cast<cf::TypeSys::VarBaseT*>(Prop->GetClientData()) : NULL;

    if (Var)
    {
        // Handle cases a2), b3) and c2).
        // There is no need to handle case b2), because it is covered by b3) as well.
        VarVisitorUpdatePropT UpdateProp(*Prop);

        Var->accept(UpdateProp);

        // The documentation at http://docs.wxwidgets.org/trunk/classwx_p_g_property.html states that unlike the
        // methods in wxPropertyGrid, wxPGProperty::SetValue() does not automatically update the display.
        // Still, it seems like an explicit call to RefreshGrid() is not needed here -- probably because RefreshGrid()
        // is automatically called by the code that generated the event that brought us here.
        // RefreshGrid();
    }
}


// This function has been duplicated into other modules, too... can we reconcile them?
static wxMenuItem* AppendMI(wxMenu& Menu, int MenuID, const wxString& Label, const wxArtID& ArtID, bool Active=true, const wxString& Help="")
{
    wxMenuItem* MI = new wxMenuItem(&Menu, MenuID, Label, Help);

    // Under wxMSW (2.9.2), the bitmap must be set before the menu item is added to the menu.
    if (ArtID != "")
        MI->SetBitmap(wxArtProvider::GetBitmap(ArtID, wxART_MENU));

    // Under wxGTK (2.9.2), the menu item must be added to the menu before we can call Enable().
    Menu.Append(MI);

    MI->Enable(Active);

    return MI;
}


void EntityInspectorDialogT::OnPropertyGridRightClick(wxPropertyGridEvent& Event)
{
    // Find the component that this right click corresponds to.
    if (m_SelectedEntity == NULL) return;

    IntrusivePtrT<cf::GameSys::ComponentBaseT> Comp = NULL;

    for (wxPGProperty* Prop = Event.GetProperty(); Prop; Prop = Prop->GetParent())
        if (Prop->IsCategory())
        {
            Comp = static_cast<cf::GameSys::ComponentBaseT*>(Prop->GetClientData());
            break;
        }

    if (Comp == NULL) return;

    const unsigned int Index = m_SelectedEntity->GetComponents().Find(Comp);

    // Fundamental components are not found in the GetComponents() list.
    const bool IsCustom = Index < m_SelectedEntity->GetComponents().Size();


    // Note that GetPopupMenuSelectionFromUser() temporarily disables UI updates for the window,
    // so our menu IDs used below should be doubly clash-free.
    enum
    {
        ID_MENU_COMPONENT_NAME = wxID_HIGHEST+1+100,
        ID_MENU_MOVE_COMPONENT_UP,
        ID_MENU_MOVE_COMPONENT_DOWN,
        ID_MENU_COPY_COMPONENT,
        ID_MENU_PASTE_COMPONENT,
        ID_MENU_REMOVE_COMPONENT,
        ID_MENU_HELP_COMPONENT
    };

    wxMenu Menu;

    AppendMI(Menu, ID_MENU_COMPONENT_NAME, wxString(Comp->GetName()) + ":", "", false);
    Menu.AppendSeparator();
    AppendMI(Menu, ID_MENU_MOVE_COMPONENT_UP,   "Move Component Up",   "list-selection-up",   IsCustom && Index > 0);
    AppendMI(Menu, ID_MENU_MOVE_COMPONENT_DOWN, "Move Component Down", "list-selection-down", IsCustom && Index+1 < m_SelectedEntity->GetComponents().Size());
    Menu.AppendSeparator();
    AppendMI(Menu, ID_MENU_COPY_COMPONENT,  "Copy Component",  wxART_COPY,  false);
    AppendMI(Menu, ID_MENU_PASTE_COMPONENT, "Paste Component", wxART_PASTE, false);
    Menu.AppendSeparator();
    AppendMI(Menu, ID_MENU_REMOVE_COMPONENT, "Remove Component", wxART_DELETE, IsCustom);
    AppendMI(Menu, ID_MENU_HELP_COMPONENT, "Help", "help-browser", true);

    switch (GetPopupMenuSelectionFromUser(Menu))
    {
        case ID_MENU_COMPONENT_NAME:
            break;

        case ID_MENU_MOVE_COMPONENT_UP:
        {
            if (!IsCustom) break;
            if (Index == 0) break;

            ArrayT<CommandT*> Commands;

            Commands.PushBack(new CommandDeleteComponentT(m_MapDocument, m_SelectedEntity, Index));
            Commands.PushBack(new CommandAddComponentT(m_MapDocument, m_SelectedEntity, Comp, Index-1));

            m_ChildFrame->SubmitCommand(new CommandMacroT(Commands, "Move component up"));
            break;
        }

        case ID_MENU_MOVE_COMPONENT_DOWN:
        {
            if (!IsCustom) break;
            if (Index+1 >= m_SelectedEntity->GetComponents().Size()) break;

            ArrayT<CommandT*> Commands;

            Commands.PushBack(new CommandDeleteComponentT(m_MapDocument, m_SelectedEntity, Index));
            Commands.PushBack(new CommandAddComponentT(m_MapDocument, m_SelectedEntity, Comp, Index+1));

            m_ChildFrame->SubmitCommand(new CommandMacroT(Commands, "Move component down"));
            break;
        }

        case ID_MENU_COPY_COMPONENT:
            break;

        case ID_MENU_PASTE_COMPONENT:
            break;

        case ID_MENU_REMOVE_COMPONENT:
        {
            if (!IsCustom) break;

            m_ChildFrame->SubmitCommand(new CommandDeleteComponentT(m_MapDocument, m_SelectedEntity, Index));
            break;
        }

        case ID_MENU_HELP_COMPONENT:
        {
            wxString cn(Comp->GetType()->ClassName);

            cn.Replace(":", "_1");

            const wxString URL = wxString("http://docs.cafu.de/lua/class") + cn + ".html";

            if (!wxLaunchDefaultBrowser(URL))
                wxMessageBox("Could not open the help URL in your default browser.", URL, wxOK | wxICON_ERROR);

            break;
        }
    }
}
