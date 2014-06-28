/*
=================================================================================
This file is part of Cafu, the open-source game engine and graphics engine
for multiplayer, cross-platform, real-time 3D action.
Copyright (C) 2002-2014 Carsten Fuchs Software.

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

#include "DialogMapCheck.hpp"
#include "CompMapEntity.hpp"
#include "MapDocument.hpp"
#include "MapEntRepres.hpp"
#include "ChildFrame.hpp"

#include "MapCommands/Select.hpp"


using namespace MapEditor;


/*
 * TODO:
 *   1. This dialog should be modeless and an observer of the world, just like our other dialogs/views/observers!
 *   2. VERY IMPORTANT: The "Fix All" button is currently dangerous - batches of commands are submitted, but what if several
 *      commands process the same entity, and e.g. the first command deletes the entity?
 *      Or the first command manipulates the properties that the subsequent commands directly or indirectly referred to?
 *      Have the ObserverT::Notify*() handlers deal with the issue? Special case code? ...?
 *      One very important requirement seems to be to only ever find and keep at most ONE problem per map entity!
 */


/// The common base class for concrete map checkers.
class MapCheckerT
{
    public:

    /// The constructor.
    MapCheckerT(MapDocumentT& MapDoc, IntrusivePtrT<CompMapEntityT> Ent) : m_MapDoc(MapDoc), m_Ent(Ent) { }

    /// The virtual destructor.
    virtual ~MapCheckerT() { }

    /// Returns the entity that this checker is assigned to.
    IntrusivePtrT<CompMapEntityT> GetEnt() const { return m_Ent; }

    /// Returns whether this map checker has actually identified a problem with the entity it is assigned to.
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

    MapDocumentT&                 m_MapDoc;
    IntrusivePtrT<CompMapEntityT> m_Ent;
};


class MC_WorldHasPlayerStartT : public MapCheckerT
{
    public:

    MC_WorldHasPlayerStartT(MapDocumentT& MapDoc, IntrusivePtrT<CompMapEntityT> Ent) : MapCheckerT(MapDoc, Ent) { }

    bool HasProblem() const
    {
        if (!m_Ent->IsWorld()) return false;

        for (unsigned long EntNr = 1 /*skip world*/; EntNr < m_MapDoc.GetEntities().Size(); EntNr++)
            if (m_MapDoc.GetEntities()[EntNr]->GetEntity()->GetComponent("PlayerStart") != NULL)
                return false;

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

    for (unsigned long EntNr = 0; EntNr < m_MapDoc.GetEntities().Size(); EntNr++)
    {
        IntrusivePtrT<CompMapEntityT> Ent = m_MapDoc.GetEntities()[EntNr];

        // IMPORTANT NOTE: Register at most ONE problem for each Ent.
        // This is supposed to avoid problems with CommandTs...
     // MC_UnknownTargetT        MC1(m_MapDoc, Ent); if (MC1.HasProblem()) { m_Problems.PushBack(new MC_UnknownTargetT       (MC1)); continue; }
     // MC_UndefinedClassOrKeysT MC2(m_MapDoc, Ent); if (MC2.HasProblem()) { m_Problems.PushBack(new MC_UndefinedClassOrKeysT(MC2)); continue; }
     // MC_DuplicateKeysT        MC3(m_MapDoc, Ent); if (MC3.HasProblem()) { m_Problems.PushBack(new MC_DuplicateKeysT       (MC3)); continue; }
     // MC_EmptySolidEntityT     MC4(m_MapDoc, Ent); if (MC4.HasProblem()) { m_Problems.PushBack(new MC_EmptySolidEntityT    (MC4)); continue; }
        MC_WorldHasPlayerStartT  MC5(m_MapDoc, Ent); if (MC5.HasProblem()) { m_Problems.PushBack(new MC_WorldHasPlayerStartT (MC5)); continue; }
    }

    // If no problems were found, add the "no problem" problem.
    if (m_Problems.Size()==0) m_Problems.PushBack(new MapCheckerT(m_MapDoc, m_MapDoc.GetRootMapEntity()));

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

    MapCheckerT*                  Problem = m_Problems[SelectionNr];
    IntrusivePtrT<CompMapEntityT> ProbEnt = Problem->GetEnt();

    ButtonFix->SetLabel(!Problem->HasProblem() ? "(is fixed)" : (Problem->CanFix() ? "Fix" : "(Can't fix)"));
    StaticTextProblemDescription->SetLabel(Problem->GetHelpText());

    ButtonGoToError->Enable(ProbEnt != NULL);
    ButtonFix      ->Enable(Problem->HasProblem() && Problem->CanFix());
    ButtonFixAll   ->Enable(Problem->CanFix());

    if (ProbEnt != NULL && !ProbEnt->IsWorld()) m_MapDoc.CompatSubmitCommand(CommandSelectT::Set(&m_MapDoc, ProbEnt->GetRepres()));
                                           else m_MapDoc.CompatSubmitCommand(CommandSelectT::Clear(&m_MapDoc));
}


void MapCheckDialogT::OnButtonGoToError(wxCommandEvent& Event)
{
    const int SelectionNr=ListBoxProblems->GetSelection();

    if (SelectionNr<0) return;

    MapCheckerT*                  Problem = m_Problems[SelectionNr];
    IntrusivePtrT<CompMapEntityT> ProbEnt = Problem->GetEnt();

    // m_MapDoc.GetChildFrame()->GetToolManager().SetActiveTool(GetToolTIM().FindTypeInfoByName("ToolSelectionT"));
    m_MapDoc.GetChildFrame()->All2DViews_Center(ProbEnt->GetRepres()->GetBB().GetCenter());
}


void MapCheckDialogT::OnButtonFix(wxCommandEvent& Event)
{
    const int SelectionNr=ListBoxProblems->GetSelection();

    if (SelectionNr<0) return;

    MapCheckerT* Problem=m_Problems[SelectionNr];
    CommandT*    ProbCmd=Problem->GetFix();

    if (ProbCmd)
    {
        m_MapDoc.CompatSubmitCommand(ProbCmd);
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
        m_MapDoc.CompatSubmitCommand(new CommandMacroT(Fixes, "Fix all: "+Problem->GetInfo()));

    wxCommandEvent CE;
    OnListBoxProblemsSelChange(CE);
}
