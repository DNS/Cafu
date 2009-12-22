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

#include "AppCaWE.hpp"
#include "CommandHistory.hpp"
#include "ParentFrame.hpp"
#include "DialogMapCheck.hpp"
#include "EntityClass.hpp"
#include "GameConfig.hpp"
#include "MapDocument.hpp"
#include "MapEntity.hpp"
#include "MapBrush.hpp"
#include "ChildFrame.hpp"
#include "ChildFrameViewWin2D.hpp"
#include "MapWorld.hpp"
#include "Options.hpp"
#include "Tool.hpp"
#include "ToolManager.hpp"
#include "EditorMaterial.hpp"
#include "EditorMaterialManager.hpp"

#include "MapCommands/DeleteProp.hpp"
#include "MapCommands/Select.hpp"
#include "wx/wx.h"

#include <typeinfo>


/*
 * TODO:
 *   1. This dialog should be modeless and an observer of the world, just like our other dialogs/views/observers!
 *   2. VERY IMPORTANT: The "Fix All" button is currently dangerous - batches of commands are submitted, but what if several
 *      commands process the same element, and e.g. the first command deletes the elem?
 *      Or the first command manipulates the properties that the subsequent commands directly or indirectly referred to?
 *      Have the ObserverT::Notify*() handlers deal with the issue? Special case code? ...?
 *      One very important requirement seems to be to only ever find and keep at most ONE problem per map element!
 *   3. Add an MP_CheckUniqueValuesT checker, to find all entity properties that are supposed to be unique, but aren't.
 *   4. This needs WAY MORE testing - make a problem-cmap test file...
 */


/// The common base class for concrete map checkers.
class MapCheckerT
{
    public:

    /// The constructor.
    MapCheckerT(MapDocumentT& MapDoc, MapElementT* Elem) : m_MapDoc(MapDoc), m_Elem(Elem) { }

    /// The virtual destructor.
    virtual ~MapCheckerT() { }

    /// Returns the element that is checker is assigned to.
    MapElementT* GetElem() const { return m_Elem; }

    /// Returns whether this map checker has actually identified a problem with the element it is assigned to.
    /// When false, there either never was a problem in the first place, or it has been fixed already.
    virtual bool HasProblem() const { return false; }

    /// Returns whether this checker is able to provide a fix the problem (some problems can only be detected, but not fixed).
    virtual bool CanFix() const { return false; }

    /// Returns a command that fixes the problem, if possible (that is, when HasProblem() and CanFix() returns true). Returns NULL otherwise.
    virtual CommandT* GetFix() const { return NULL; }

    /// Returns a brief information about the nature of the problem.
    virtual wxString GetInfo() const { return "No problem found."; }

    /// Returns a longer help text about the details of the problem and how it can be fixed.
    virtual wxString GetHelpText() const { return GetInfo(); }


    protected:

    MapDocumentT& m_MapDoc;
    MapElementT*  m_Elem;
};


class MC_UnknownTargetT : public MapCheckerT
{
    public:

    MC_UnknownTargetT(MapDocumentT& MapDoc, MapElementT* Elem) : MapCheckerT(MapDoc, Elem) { }

    bool HasProblem() const
    {
        // If m_Elem is not an entity, we don't have a problem here.
        if (m_Elem->GetType()!=&MapEntityT::TypeInfo) return false;

        const MapEntityT*   Ent1      =static_cast<MapEntityT*>(m_Elem);
        const EntPropertyT* TargetProp=Ent1->FindProperty("target");

        // If this entity doesn't have the "target" property or an empty value, we don't have a problem.
        if (TargetProp==NULL) return false;
        if (TargetProp->Value=="") return false;

        // Finally search all entities in the world for a matching "name" property.
        for (unsigned long Ent2Nr=1/*skip world*/; Ent2Nr<m_MapDoc.GetEntities().Size(); Ent2Nr++)
        {
            const EntPropertyT* NameProp=m_MapDoc.GetEntities()[Ent2Nr]->FindProperty("name");

            if (NameProp==NULL) continue;
            if (NameProp->Value==TargetProp->Value) return false;
        }

        return true;
    }

    wxString GetTargetValue() const
    {
        if (m_Elem->GetType()!=&MapEntityT::TypeInfo) return "";

        const MapEntityT*   Ent       =static_cast<MapEntityT*>(m_Elem);
        const EntPropertyT* TargetProp=Ent->FindProperty("target");

        return TargetProp ? TargetProp->Value : "";
    }

    wxString GetInfo() const { return "Unmatched entity target."; }
    wxString GetHelpText() const { return "The \"target\" property of this entity has value \""+GetTargetValue()+"\", but there is no entity with a corresponding name."; }
};


static bool LargestFirst(int const& elem1, int const& elem2)
{
    return elem2<elem1;
}


class MC_UndefinedClassOrKeysT : public MapCheckerT
{
    public:

    MC_UndefinedClassOrKeysT(MapDocumentT& MapDoc, MapElementT* Elem)
        : MapCheckerT(MapDoc, Elem),
          m_Ent(NULL)
    {
        m_Ent=dynamic_cast<MapEntityT*>(m_Elem);
    }

    bool HasProblem() const
    {
        if (!m_Ent) return false;
        if (!m_Ent->GetClass()->IsInGameConfig()) return true;

        return GetUndefKeys().Size()>0;   // This is inefficient...
    }

    bool CanFix() const
    {
        return m_Ent && m_Ent->GetClass()->IsInGameConfig() && GetUndefKeys().Size()>0;
    }

    CommandT* GetFix() const
    {
        if (!m_Ent) return NULL;
        if (!m_Ent->GetClass()->IsInGameConfig()) return NULL;

        const ArrayT<EntPropertyT>& Props=m_Ent->GetProperties();
        ArrayT<int>                 DelIndices;

        for (unsigned long PropNr=0; PropNr<Props.Size(); PropNr++)
            if (m_Ent->GetClass()->FindVar(Props[PropNr].Key)==NULL)
                DelIndices.PushBack(PropNr);

        if (DelIndices.Size()==0) return NULL;
        if (DelIndices.Size()==1) return new CommandDeletePropertyT(m_MapDoc, m_Ent, DelIndices[0]);

        ArrayT<CommandT*> Commands;
        DelIndices.QuickSort(LargestFirst);     // Must delete by index strictly in largest-first order!
        for (unsigned long iNr=0; iNr<DelIndices.Size(); iNr++)
            Commands.PushBack(new CommandDeletePropertyT(m_MapDoc, m_Ent, DelIndices[iNr]));

        return new CommandMacroT(Commands, "Delete unused entity keys.");
    }

    ArrayT<wxString> GetUndefKeys() const
    {
        ArrayT<wxString> UndefKeys;

        if (m_Ent)
        {
            const ArrayT<EntPropertyT>& Props=m_Ent->GetProperties();

            for (unsigned long PropNr=0; PropNr<Props.Size(); PropNr++)
                if (m_Ent->GetClass()->FindVar(Props[PropNr].Key)==NULL)
                    UndefKeys.PushBack(Props[PropNr].Key);
        }

        return UndefKeys;
    }

    wxString GetInfo() const
    {
        if (!m_Ent) return "";

        return m_Ent->GetClass()->IsInGameConfig() ? "Undefined entity keys." : "Undefined entity class.";
    }

    wxString GetHelpText() const
    {
        if (!m_Ent) return "";

        if (!m_Ent->GetClass()->IsInGameConfig())
            return "The class \""+m_Ent->GetClass()->GetName()+"\" of this entity is undefined in the game configuration of this map.";

        wxString Text="This entity has properties with keys that are undefined in its class \""+m_Ent->GetClass()->GetName()+"\", and are thus unused:\n\n";
        const ArrayT<wxString> UndefKeys=GetUndefKeys();

        for (unsigned long KeyNr=0; KeyNr<UndefKeys.Size(); KeyNr++)
        {
            Text+=UndefKeys[KeyNr];
            if (KeyNr+1<UndefKeys.Size()) Text+=", ";
        }

        Text+="\n\nThis problem is often a harmless by-product of importing maps from other games.";
        return Text;
    }


    private:

    MapEntityT* m_Ent;
};


class MC_DuplicateKeysT : public MapCheckerT
{
    public:

    MC_DuplicateKeysT(MapDocumentT& MapDoc, MapElementT* Elem) : MapCheckerT(MapDoc, Elem) { }

    bool HasProblem() const
    {
        if (m_Elem->GetType()!=&MapEntityT::TypeInfo) return false;

        const MapEntityT*           Ent  =static_cast<MapEntityT*>(m_Elem);
        const ArrayT<EntPropertyT>& Props=Ent->GetProperties();

        for (unsigned long i=0; i<Props.Size(); i++)
            for (unsigned long j=i+1; j<Props.Size(); j++)
                if (Props[i].Key==Props[j].Key)
                    return true;

        return false;
    }

    bool CanFix() const { return true; }

    CommandT* GetFix() const
    {
        if (m_Elem->GetType()!=&MapEntityT::TypeInfo) return NULL;

        MapEntityT*                 Ent  =static_cast<MapEntityT*>(m_Elem);
        const ArrayT<EntPropertyT>& Props=Ent->GetProperties();
        ArrayT<int>                 DelIndices;

        for (unsigned long i=0; i<Props.Size(); i++)
            for (unsigned long j=i+1; j<Props.Size(); j++)
                if (Props[i].Key==Props[j].Key)
                    if (DelIndices.Find(j)==-1)
                        DelIndices.PushBack(j);

        if (DelIndices.Size()==0) return NULL;
        if (DelIndices.Size()==1) return new CommandDeletePropertyT(m_MapDoc, Ent, DelIndices[0]);

        ArrayT<CommandT*> Commands;
        DelIndices.QuickSort(LargestFirst);     // Must delete by index strictly in largest-first order!
        for (unsigned long iNr=0; iNr<DelIndices.Size(); iNr++)
            Commands.PushBack(new CommandDeletePropertyT(m_MapDoc, Ent, DelIndices[iNr]));

        return new CommandMacroT(Commands, "Delete duplicate entity keys.");
    }

    wxString GetInfo() const { return "Duplicate entity keys."; }
    wxString GetHelpText() const { return "This entity has properties with keys that occur multiply."; }
};


class MC_EmptySolidEntityT : public MapCheckerT
{
    public:

    MC_EmptySolidEntityT(MapDocumentT& MapDoc, MapElementT* Elem) : MapCheckerT(MapDoc, Elem) { }

    bool HasProblem() const
    {
        if (m_Elem->GetType()!=&MapEntityT::TypeInfo) return false;

        const MapEntityT* Ent=static_cast<MapEntityT*>(m_Elem);

        return Ent->GetClass()->IsSolidClass() && Ent->GetPrimitives().Size()==0;
    }

    wxString GetInfo() const { return "Empty solid entity."; }
    wxString GetHelpText() const { return "This entity is supposed to be composed of map primitives (brushes, patches, models, ...), but has none. Fixing the error deletes the empty entity."; }
};


class MC_WorldHasPlayerStartT : public MapCheckerT
{
    public:

    MC_WorldHasPlayerStartT(MapDocumentT& MapDoc, MapElementT* Elem) : MapCheckerT(MapDoc, Elem) { }

    bool HasProblem() const
    {
        if (m_Elem->GetType()!=&MapWorldT::TypeInfo) return false;

        for (unsigned long EntNr=1/*skip world*/; EntNr<m_MapDoc.GetEntities().Size(); EntNr++)
            if (m_MapDoc.GetEntities()[EntNr]->GetClass()->GetName()=="info_player_start") return false;

        return true;
    }

    wxString GetInfo() const { return "No player start."; }
    wxString GetHelpText() const { return "There is no player start in the map. Use the \"New Entity\" tool in order to add one."; }
};


BEGIN_EVENT_TABLE(MapCheckDialogT, wxDialog)
    EVT_LISTBOX(MapCheckDialogT::ID_LISTBOX_PROBLEMS, MapCheckDialogT::OnListBoxProblemsSelChange)
    EVT_BUTTON(MapCheckDialogT::ID_BUTTON_GOTO_ERROR, MapCheckDialogT::OnButtonGoToError)
    EVT_BUTTON(MapCheckDialogT::ID_BUTTON_FIX, MapCheckDialogT::OnButtonFix)
    EVT_BUTTON(MapCheckDialogT::ID_BUTTON_FIXALL, MapCheckDialogT::OnButtonFixAll)
END_EVENT_TABLE()


MapCheckDialogT::MapCheckDialogT(wxWindow* Parent, MapDocumentT& MapDoc)
    : wxDialog(Parent, -1, "Map Problems Checker", wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
      m_MapDoc(MapDoc)
{
    wxBoxSizer *item0 = new wxBoxSizer( wxVERTICAL );

    wxStaticText *item1 = new wxStaticText(this, -1, wxT("Problems found:"), wxDefaultPosition, wxDefaultSize, 0 );
    item0->Add( item1, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxTOP, 5 );

    ListBoxProblems=new wxListBox(this, ID_LISTBOX_PROBLEMS, wxDefaultPosition, wxSize(200,100), 0, NULL, wxLB_SINGLE );
    item0->Add(ListBoxProblems, 1, wxGROW|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxBOTTOM, 5 );

    wxStaticBox *item4 = new wxStaticBox(this, -1, wxT("Description") );
    wxStaticBoxSizer *item3 = new wxStaticBoxSizer( item4, wxHORIZONTAL );

    StaticTextProblemDescription=new wxStaticText(this, -1, wxT("The description of the selected problem."), wxDefaultPosition, wxDefaultSize, wxST_NO_AUTORESIZE | wxSUNKEN_BORDER);
    item3->Add(StaticTextProblemDescription, 1, wxGROW|wxALIGN_CENTER_HORIZONTAL|wxALL, 5);

    wxBoxSizer *item6 = new wxBoxSizer( wxVERTICAL );

    ButtonGoToError=new wxButton(this, ID_BUTTON_GOTO_ERROR, wxT("Go to"), wxDefaultPosition, wxDefaultSize, 0 );
    ButtonGoToError->SetToolTip("Centers the 2D views on the related map element.");
    item6->Add(ButtonGoToError, 0, wxALIGN_CENTER|wxALL, 5 );

    ButtonFix= new wxButton(this, ID_BUTTON_FIX, wxT("Fix"), wxDefaultPosition, wxDefaultSize, 0 );
    ButtonFix->SetToolTip("Automatically fixes the currently selected problem, if possible.");
    item6->Add(ButtonFix, 0, wxALIGN_CENTER|wxALL, 5 );

    ButtonFixAll= new wxButton(this, ID_BUTTON_FIXALL, wxT("Fix all"), wxDefaultPosition, wxDefaultSize, 0 );
    ButtonFixAll->SetToolTip("Automatically fixes all problems of the same type, if possible.");
    item6->Add(ButtonFixAll, 0, wxALIGN_CENTER|wxALL, 5 );

    item3->Add( item6, 0, wxALIGN_CENTER_HORIZONTAL, 5 );

    item0->Add( item3, 1, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 5 );

    wxButton *item10 = new wxButton(this, wxID_CANCEL, wxT("Close"), wxDefaultPosition, wxDefaultSize, 0 );
    item10->SetDefault();
    item0->Add( item10, 0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL|wxALL, 5 );

    this->SetSizer(item0);
    item0->SetSizeHints(this);

    UpdateProblems();
}


MapCheckDialogT::~MapCheckDialogT()
{
    for (unsigned long ProblemNr=0; ProblemNr<m_Problems.Size(); ProblemNr++)
        delete m_Problems[ProblemNr];
    m_Problems.Clear();
}


void MapCheckDialogT::UpdateProblems()
{
    for (unsigned long ProblemNr=0; ProblemNr<m_Problems.Size(); ProblemNr++)
        delete m_Problems[ProblemNr];
    m_Problems.Overwrite();

    ArrayT<MapElementT*> Elems;
    m_MapDoc.GetAllElems(Elems);

    for (unsigned long ElemNr=0; ElemNr<Elems.Size(); ElemNr++)
    {
        MapElementT* Elem=Elems[ElemNr];

        // IMPORTANT NOTE: Register at most ONE problem for each Elem.
        // This is supposed to avoid problems with CommandTs...
        MC_UnknownTargetT        MC1(m_MapDoc, Elem); if (MC1.HasProblem()) { m_Problems.PushBack(new MC_UnknownTargetT       (MC1)); continue; }
        MC_UndefinedClassOrKeysT MC2(m_MapDoc, Elem); if (MC2.HasProblem()) { m_Problems.PushBack(new MC_UndefinedClassOrKeysT(MC2)); continue; }
        MC_DuplicateKeysT        MC3(m_MapDoc, Elem); if (MC3.HasProblem()) { m_Problems.PushBack(new MC_DuplicateKeysT       (MC3)); continue; }
        MC_EmptySolidEntityT     MC4(m_MapDoc, Elem); if (MC4.HasProblem()) { m_Problems.PushBack(new MC_EmptySolidEntityT    (MC4)); continue; }
        MC_WorldHasPlayerStartT  MC5(m_MapDoc, Elem); if (MC5.HasProblem()) { m_Problems.PushBack(new MC_WorldHasPlayerStartT (MC5)); continue; }
    }

    // If no problems were found, add the "no problem" problem.
    if (m_Problems.Size()==0) m_Problems.PushBack(new MapCheckerT(m_MapDoc, m_MapDoc.GetEntities()[0]));

    // Fill the wxListBox.
    ListBoxProblems->Clear();
    for (unsigned long ProblemNr=0; ProblemNr<m_Problems.Size(); ProblemNr++)
        ListBoxProblems->Append(wxString::Format("%lu ", ProblemNr+1)+m_Problems[ProblemNr]->GetInfo());

    ListBoxProblems->SetSelection(0);
    wxCommandEvent CE; OnListBoxProblemsSelChange(CE);
}


void MapCheckDialogT::OnListBoxProblemsSelChange(wxCommandEvent& Event)
{
    const int SelectionNr=ListBoxProblems->GetSelection();

    if (SelectionNr<0)
    {
        ButtonFix->SetLabel("Fix");
        StaticTextProblemDescription->SetLabel("");

        ButtonGoToError->Disable();
        ButtonFix      ->Disable();
        ButtonFixAll   ->Disable();
        return;
    }

    MapCheckerT* Problem =m_Problems[SelectionNr];
    MapElementT* ProbElem=Problem->GetElem();

    ButtonFix->SetLabel(!Problem->HasProblem() ? "(is fixed)" : (Problem->CanFix() ? "Fix" : "(Can't fix)"));
    StaticTextProblemDescription->SetLabel(Problem->GetHelpText());

    ButtonGoToError->Enable(ProbElem!=NULL);
    ButtonFix      ->Enable(Problem->HasProblem() && Problem->CanFix());
    ButtonFixAll   ->Enable(Problem->CanFix());

    if (ProbElem && ProbElem->GetType()!=&MapWorldT::TypeInfo) m_MapDoc.GetHistory().SubmitCommand(CommandSelectT::Set(&m_MapDoc, ProbElem));
                                                          else m_MapDoc.GetHistory().SubmitCommand(CommandSelectT::Clear(&m_MapDoc));
}


void MapCheckDialogT::OnButtonGoToError(wxCommandEvent& Event)
{
    const int SelectionNr=ListBoxProblems->GetSelection();

    if (SelectionNr<0) return;

    MapCheckerT* Problem =m_Problems[SelectionNr];
    MapElementT* ProbElem=Problem->GetElem();

    // m_MapDoc.GetChildFrame()->GetToolManager().SetActiveTool(GetToolTIM().FindTypeInfoByName("ToolSelectionT"));
    m_MapDoc.GetChildFrame()->All2DViews_Center(ProbElem->GetBB().GetCenter());
}


void MapCheckDialogT::OnButtonFix(wxCommandEvent& Event)
{
    const int SelectionNr=ListBoxProblems->GetSelection();

    if (SelectionNr<0) return;

    MapCheckerT* Problem=m_Problems[SelectionNr];
    CommandT*    ProbCmd=Problem->GetFix();

    if (ProbCmd)
    {
        m_MapDoc.GetHistory().SubmitCommand(ProbCmd);
    }

    wxCommandEvent CE;
    OnListBoxProblemsSelChange(CE);
}


void MapCheckDialogT::OnButtonFixAll(wxCommandEvent& Event)
{
    const int SelectionNr=ListBoxProblems->GetSelection();

    if (SelectionNr<0) return;

    wxBusyCursor      BusyCursor;
    MapCheckerT*      Problem=m_Problems[SelectionNr];
    ArrayT<CommandT*> Fixes;

    for (unsigned long ProblemNr=0; ProblemNr<m_Problems.Size(); ProblemNr++)
        if (typeid(Problem)==typeid(m_Problems[ProblemNr]))
        {
            CommandT* ProbCmd=m_Problems[ProblemNr]->GetFix();

            if (ProbCmd) Fixes.PushBack(ProbCmd);
        }

    if (Fixes.Size())
        m_MapDoc.GetHistory().SubmitCommand(new CommandMacroT(Fixes, "Fix all: "+Problem->GetInfo()));

    wxCommandEvent CE;
    OnListBoxProblemsSelChange(CE);
}
