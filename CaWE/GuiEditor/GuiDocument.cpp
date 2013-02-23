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

#include "GuiDocument.hpp"
#include "Windows/EditorWindow.hpp"
#include "../GameConfig.hpp"
#include "../EditorMaterialEngine.hpp"

#include "GuiSys/CompBase.hpp"
#include "GuiSys/GuiImpl.hpp"
#include "GuiSys/VarVisitorsLua.hpp"
#include "TypeSys.hpp"

#include <fstream>


using namespace GuiEditor;


GuiPropertiesT::GuiPropertiesT(cf::GuiSys::GuiImplT& Gui)
    : Activate(Gui.GetIsActive()),
      Interactive(Gui.GetIsInteractive()),
      ShowMouse(Gui.IsMouseShown()),
      DefaultFocus(Gui.GetFocusWindow()!=NULL ? Gui.GetFocusWindow()->GetName() : "")
{
}


GuiDocumentT::GuiDocumentT(GameConfigT* GameConfig, const wxString& GuiInitFileName)
    : m_Gui(NULL),
      m_RootWindow(NULL),
      m_Selection(),
      m_EditorMaterials(),
      m_GameConfig(GameConfig)
{
    // Load GUI initialization script and create documents window hierarchy.
    if (GuiInitFileName!="")
    {
        const std::string gifn(GuiInitFileName);

        m_Gui=new cf::GuiSys::GuiImplT(GameConfig->GetGuiResources(), gifn, cf::GuiSys::GuiImplT::InitFlag_InGuiEditor);

        if (m_Gui->GetScriptInitResult()!="")
        {
            // GuiImplT::InitErrorT excecptions are caught in the caller code.
            // Here we handle the case that initializing the GUI succeeded "halfway".
            wxMessageBox("GUI file "+GuiInitFileName+" was loaded, but errors occurred:\n\n"+
                         m_Gui->GetScriptInitResult()+"\n\n"+
                         "You may choose to ignore this error and proceed,\n"+
                         "but it is probably better to edit the file and manually fix the problem first.",
                         "GUI initialization warning", wxOK | wxICON_EXCLAMATION);
        }

        m_GuiProperties=GuiPropertiesT(*m_Gui);

        // Clone root window and all of its children.
        m_RootWindow=m_Gui->GetRootWindow()->Clone(true);
    }
    else
    {
        m_Gui=new cf::GuiSys::GuiImplT(GameConfig->GetGuiResources(),
            "Win=gui:new('WindowT'); gui:SetRootWindow(Win); gui:showMouse(false); gui:setFocus(Win); Win:SetName('Root'); Win:set(\"rect\", 0, 0, 640, 480);",
            cf::GuiSys::GuiImplT::InitFlag_InlineCode | cf::GuiSys::GuiImplT::InitFlag_InGuiEditor);

        m_GuiProperties=GuiPropertiesT(*m_Gui);

        // Clone root window and all of its children.
        m_RootWindow=m_Gui->GetRootWindow()->Clone(true);
    }

    m_Selection.PushBack(m_RootWindow); // Make root window default selection.

    // Create editor windows for all GuiSys windows.
    CreateSibling(m_RootWindow, this);
    GetSibling(m_RootWindow)->SetSelected(true);

    ArrayT< IntrusivePtrT<cf::GuiSys::WindowT> > GuiWindows;
    m_RootWindow->GetChildren(GuiWindows, true);

    for (unsigned long i=0; i<GuiWindows.Size(); i++)
        CreateSibling(GuiWindows[i], this);


    // Init the editor materials.
    const std::map<std::string, MaterialT*>& GuiMaterials=m_Gui->GetMaterialManager().GetAllMaterials();

    // No need to explicitly sort the m_EditorMaterials array after it has been filled in the order of the std::map.
    for (std::map<std::string, MaterialT*>::const_iterator It=GuiMaterials.begin(); It!=GuiMaterials.end(); It++)
        m_EditorMaterials.PushBack(new EngineMaterialT(It->second));
}


GuiDocumentT::~GuiDocumentT()
{
    for (unsigned long MatNr=0; MatNr<m_EditorMaterials.Size(); MatNr++)
        delete m_EditorMaterials[MatNr];

    m_RootWindow=NULL;
    delete m_Gui;
}


void GuiDocumentT::SetSelection(const ArrayT< IntrusivePtrT<cf::GuiSys::WindowT> >& NewSelection)
{
    // Clear the previous selection.
    for (unsigned long SelNr=0; SelNr<m_Selection.Size(); SelNr++)
        GetSibling(m_Selection[SelNr])->SetSelected(false);

    m_Selection.Clear();

    for (unsigned long NewSelNr=0; NewSelNr<NewSelection.Size(); NewSelNr++)
    {
        m_Selection.PushBack(NewSelection[NewSelNr]);

        GetSibling(NewSelection[NewSelNr])->SetSelected(true);
    }
}


// Recursively saves the window instantiation of the passed window and all of its children.
static void SaveWindowInstantiation(std::ostream& OutFile, IntrusivePtrT<cf::GuiSys::WindowT> Window, const wxString& ParentName)
{
    if (GuiDocumentT::GetSibling(Window)->ConvertToComponent())
    {
        // Converting to components: use "WindowT", even for derived classes.
        OutFile << ParentName + Window->GetName() << "=gui:new(\"" << "WindowT" << "\", \"" << Window->GetName() << "\");\n";
    }
    else
    {
        OutFile << ParentName + Window->GetName() << "=gui:new(\"" << Window->GetType()->ClassName << "\", \"" << Window->GetName() << "\");\n";
    }

    if (Window==Window->GetRoot()) OutFile << "\n";

    const wxString NewParentName = Window->GetRoot() == Window ? "" : ParentName + Window->GetName() + ".";

    for (unsigned long ChildNr=0; ChildNr<Window->GetChildren().Size(); ChildNr++)
        SaveWindowInstantiation(OutFile, Window->GetChildren()[ChildNr], NewParentName);

    if (Window->GetParent()==Window->GetRoot()) OutFile << "\n";
}


// Recursively saves the window hierarchy of the passed window and all of its children.
static void SaveWindowHierarchy(std::ostream& OutFile, IntrusivePtrT<cf::GuiSys::WindowT> Window, const wxString& ParentName)
{
    if (Window!=Window->GetRoot()) // Root window is saved separately (see Save_cgui).
    {
        wxString HierarchyParent=ParentName;

        if (HierarchyParent=="") HierarchyParent=Window->GetRoot()->GetName();
        else                     HierarchyParent=HierarchyParent.substr(0, HierarchyParent.size()-1); // Strip parent of its last '.' character.

        OutFile << HierarchyParent << ":AddChild(" << ParentName + Window->GetName() << ");\n";
    }

    wxString NewParentName = Window->GetRoot() == Window ? "" : ParentName + Window->GetName() + ".";

    for (unsigned long ChildNr=0; ChildNr<Window->GetChildren().Size(); ChildNr++)
        SaveWindowHierarchy(OutFile, Window->GetChildren()[ChildNr], NewParentName);

    if (Window->GetParent() == Window->GetRoot()) OutFile << "\n";
}


static void SaveComponents(std::ostream& OutFile, IntrusivePtrT<cf::GuiSys::WindowT> Window)
{
    if (Window->GetComponents().Size() == 0)
        return;

    cf::GuiSys::VarVisitorToLuaCodeT ToLua(OutFile);

    OutFile << "\n";

    for (unsigned int CompNr = 1; CompNr <= Window->GetComponents().Size(); CompNr++)
    {
        IntrusivePtrT<cf::GuiSys::ComponentBaseT> Comp = Window->GetComponents()[CompNr - 1];
        const ArrayT<cf::TypeSys::VarBaseT*>&     Vars = Comp->GetMemberVars().GetArray();

        OutFile << "    local c" << CompNr << " = gui:new(\"" << Comp->GetType()->ClassName << "\")\n";

        for (unsigned int VarNr = 0; VarNr < Vars.Size(); VarNr++)
        {
            const cf::TypeSys::VarBaseT* Var = Vars[VarNr];

            OutFile << "    c" << CompNr << ":set(\"" << Var->GetName() << "\", ";
            Var->accept(ToLua);
            OutFile << ")\n";
        }

        OutFile << "\n";
    }

    OutFile << "    self:AddComponent(";
    for (unsigned int CompNr = 1; CompNr <= Window->GetComponents().Size(); CompNr++)
    {
        OutFile << "c" << CompNr;
        if (CompNr < Window->GetComponents().Size()) OutFile << ", ";
    }
    OutFile << ")\n";
}


// Saves the window initialization method for the root window.
static void SaveRootInitialization(std::ostream& OutFile, IntrusivePtrT<cf::GuiSys::WindowT> Root, const GuiPropertiesT& GuiProps)
{
    wxASSERT(Root==Root->GetRoot());

    OutFile << "function " << Root->GetName() << ":OnInit()\n";

    GuiDocumentT::GetSibling(Root)->WriteInitMethod(OutFile);
    SaveComponents(OutFile, Root);

    OutFile << "\n";
    OutFile << "    gui:activate      (" << (GuiProps.Activate    ? "true" : "false") << ");\n";
    OutFile << "    gui:setInteractive(" << (GuiProps.Interactive ? "true" : "false") << ");\n";
    OutFile << "    gui:showMouse     (" << (GuiProps.ShowMouse   ? "true" : "false") << ");\n";

    if (GuiProps.DefaultFocus!="")
    {
        IntrusivePtrT<cf::GuiSys::WindowT> FocusWin = Root->Find(std::string(GuiProps.DefaultFocus));

        if (FocusWin != NULL)
        {
            std::string FocusName = FocusWin->GetName();

            while (true)
            {
                FocusWin = FocusWin->GetParent();
                if (FocusWin == NULL) break;
                if (FocusWin == Root) break;    // This is because the child windows of Root are created top-level in the script, i.e. "ChildName" instead of "RootName.ChildName".
                FocusName = FocusWin->GetName() + "." + FocusName;
            }

            OutFile << "    gui:setFocus      (" <<  FocusName << ");\n";
        }
    }

    OutFile << "end\n\n";
}


// Recursively saves the window initialization function of the window passed and all of its children.
static void SaveWindowInitialization(std::ostream& OutFile, IntrusivePtrT<cf::GuiSys::WindowT> Window, const wxString& ParentName)
{
    if (Window!=Window->GetRoot())
    {
        OutFile << "function " << ParentName + Window->GetName() << ":OnInit()\n";

        GuiDocumentT::GetSibling(Window)->WriteInitMethod(OutFile);
        SaveComponents(OutFile, Window);

        OutFile << "end\n\n";
    }

    const wxString NewParentName = Window->GetRoot() == Window ? "" : ParentName + Window->GetName() + ".";

    for (unsigned long ChildNr=0; ChildNr<Window->GetChildren().Size(); ChildNr++)
        SaveWindowInitialization(OutFile, Window->GetChildren()[ChildNr], NewParentName);
}


bool GuiDocumentT::SaveInit_cgui(std::ostream& OutFile)
{
    OutFile << "-- This is a Cafu engine GUI script file, written by CaWE, the Cafu World Editor.\n";
    OutFile << "-- You CAN edit this file manually, but note that CaWE may overwrite your changes.\n";
    OutFile << "-- It is recommended that you place all your customizations like method overrides\n";
    OutFile << "-- and effects into a separate .cgui file that calls dofile() for including this.\n";
    OutFile << "\n\n";
    OutFile << "-- Instantiation of all windows.\n";
    OutFile << "-- *****************************\n";
    OutFile << "\n";

    SaveWindowInstantiation(OutFile, m_RootWindow, "");

    OutFile << "\n";
    OutFile << "-- Set the GUIs root window.\n";
    OutFile << "-- *************************\n";
    OutFile << "\n";

    OutFile << "gui:SetRootWindow(" << m_RootWindow->GetName() << ");\n";

    OutFile << "\n\n";
    OutFile << "-- Setup the window hierarchy.\n";
    OutFile << "-- ***************************\n";
    OutFile << "\n";

    SaveWindowHierarchy(OutFile, m_RootWindow, "");

    OutFile << "\n";
    OutFile << "-- Initialization of the window contents (\"constructor code\").\n";
    OutFile << "-- ***********************************************************\n";
    OutFile << "\n";

    SaveRootInitialization(OutFile, m_RootWindow, m_GuiProperties);
    SaveWindowInitialization(OutFile, m_RootWindow, "");

    if (OutFile.fail())
    {
        wxMessageBox("There was a file error.", "Message", wxOK | wxICON_EXCLAMATION);
        return false;
    }

    return true;
}


#include "Windows/EditorChoiceWindow.hpp"
#include "Windows/EditorEditWindow.hpp"
#include "Windows/EditorListBoxWindow.hpp"
#include "Windows/EditorModelWindow.hpp"

#include "GuiSys/WindowChoice.hpp"
#include "GuiSys/WindowEdit.hpp"
#include "GuiSys/WindowListBox.hpp"
#include "GuiSys/WindowModel.hpp"


/*static*/ void GuiDocumentT::CreateSibling(IntrusivePtrT<cf::GuiSys::WindowT> Win, GuiDocumentT* GuiDoc)
{
    wxASSERT(!Win.IsNull());

    IntrusivePtrT<cf::GuiSys::ChoiceT>      Choice     =dynamic_pointer_cast<cf::GuiSys::ChoiceT>(Win);
    IntrusivePtrT<cf::GuiSys::EditWindowT>  EditWindow =dynamic_pointer_cast<cf::GuiSys::EditWindowT>(Win);
    IntrusivePtrT<cf::GuiSys::ListBoxT>     ListBox    =dynamic_pointer_cast<cf::GuiSys::ListBoxT>(Win);
    IntrusivePtrT<cf::GuiSys::ModelWindowT> ModelWindow=dynamic_pointer_cast<cf::GuiSys::ModelWindowT>(Win);

    EditorWindowT* EditorWin=NULL;

         if (!Choice.IsNull())      EditorWin=new EditorChoiceWindowT(Choice, GuiDoc);
    else if (!EditWindow.IsNull())  EditorWin=new EditorEditWindowT(EditWindow, GuiDoc);
    else if (!ListBox.IsNull())     EditorWin=new EditorListBoxWindowT(ListBox, GuiDoc);
    else if (!ModelWindow.IsNull()) EditorWin=new EditorModelWindowT(ModelWindow, GuiDoc);
    else                            EditorWin=new EditorWindowT(Win, GuiDoc);

    Win->SetExtData(EditorWin);
}


/*static*/ EditorWindowT* GuiDocumentT::GetSibling(IntrusivePtrT<cf::GuiSys::WindowT> Win)
{
    wxASSERT(dynamic_cast<EditorWindowT*>(Win->GetExtData()));

    return static_cast<EditorWindowT*>(Win->GetExtData());
}
