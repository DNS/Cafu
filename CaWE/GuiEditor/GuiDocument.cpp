/*
=================================================================================
This file is part of Cafu, the open-source game engine and graphics engine
for multiplayer, cross-platform, real-time 3D action.
Copyright (C) 2002-2011 Carsten Fuchs Software.

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
#include "EditorData/Window.hpp"
#include "../GameConfig.hpp"
#include "../EditorMaterialEngine.hpp"

#include "GuiSys/GuiImpl.hpp"
#include "GuiSys/Window.hpp"
#include "TypeSys.hpp"

#include <fstream>


using namespace GuiEditor;


GuiPropertiesT::GuiPropertiesT(cf::GuiSys::GuiImplT& Gui)
    : Activate(Gui.GetIsActive()),
      Interactive(Gui.GetIsInteractive()),
      ShowMouse(Gui.IsMouseShown()),
      DefaultFocus(Gui.GetFocusWindow()!=NULL ? Gui.GetFocusWindow()->Name : "")
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

        m_Gui=new cf::GuiSys::GuiImplT(gifn);

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
        m_Gui=new cf::GuiSys::GuiImplT("Win=gui:new('WindowT'); gui:SetRootWindow(Win); gui:showMouse(false); gui:setFocus(Win); Win:SetName('Window'); Win:set(\"rect\", 0, 0, 640, 480);", true);

        m_GuiProperties=GuiPropertiesT(*m_Gui);

        // Clone root window and all of its children.
        m_RootWindow=m_Gui->GetRootWindow()->Clone(true);
    }

    m_Selection.PushBack(m_RootWindow); // Make root window default selection.

    // Create editor data for all windows.
    new EditorDataWindowT(m_RootWindow, this);
    ((EditorDataWindowT*)m_RootWindow->EditorData)->Selected=true; // Mark root as selected.

    ArrayT<cf::GuiSys::WindowT*> GuiWindows;
    m_RootWindow->GetChildren(GuiWindows, true);

    for (unsigned long i=0; i<GuiWindows.Size(); i++)
        new EditorDataWindowT(GuiWindows[i], this);


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

    ArrayT<cf::GuiSys::WindowT*> Children;
    m_RootWindow->GetChildren(Children, true);

    for (unsigned long i=0; i<Children.Size(); i++)
        delete Children[i];

    delete m_RootWindow;
    delete m_Gui;
}


cf::GuiSys::WindowT* GuiDocumentT::FindWindowByName(const wxString& WindowName)
{
    if (m_RootWindow->GetName()==WindowName) return m_RootWindow;

    return m_RootWindow->Find(std::string(WindowName));
}


void GuiDocumentT::SetSelection(const ArrayT<cf::GuiSys::WindowT*>& NewSelection)
{
    // Clear the previous selection.
    for (unsigned long SelNr=0; SelNr<m_Selection.Size(); SelNr++)
        ((EditorDataWindowT*)m_Selection[SelNr]->EditorData)->Selected=false;

    m_Selection.Clear();

    for (unsigned long NewSelNr=0; NewSelNr<NewSelection.Size(); NewSelNr++)
    {
        m_Selection.PushBack(NewSelection[NewSelNr]);

        ((EditorDataWindowT*)NewSelection[NewSelNr]->EditorData)->Selected=true;
    }
}


const ArrayT<cf::GuiSys::WindowT*>& GuiDocumentT::GetSelection()
{
    return m_Selection;
}


// Recursively makes sure that the children of each window have unique names.
// Normally the other GUI editor code should make sure that that is always true, but right now it
// is possible to create violations of this constraint via drag-and-drop in the window hierarchy tree
// (can drop a window as a child of another window that already has a child with the same name).
static void CheckWindowNames(cf::GuiSys::WindowT* Window)
{
    const std::string OldName=Window->Name;

    ((EditorDataWindowT*)Window->EditorData)->RepairNameUniqueness();

    if (Window->Name!=OldName)
        ((EditorDataWindowT*)Window->EditorData)->GetGuiDoc()->UpdateAllObservers_Modified(Window, WMD_PROPERTY_CHANGED, "Name");

    for (unsigned long ChildNr=0; ChildNr<Window->Children.Size(); ChildNr++)
        CheckWindowNames(Window->Children[ChildNr]);
}


// Recursively saves the window instantiation of the passed window and all of its children.
static void SaveWindowInstantiation(std::ostream& OutFile, cf::GuiSys::WindowT* Window, const wxString& ParentName)
{
    OutFile << ParentName+Window->Name << "=gui:new(\"" << Window->GetType()->ClassName << "\", \"" << Window->Name << "\");\n";

    if (Window==Window->GetRoot()) OutFile << "\n";

    wxString NewParentName=Window->GetRoot()==Window ? "" : ParentName+Window->Name+".";

    for (unsigned long ChildNr=0; ChildNr<Window->Children.Size(); ChildNr++)
        SaveWindowInstantiation(OutFile, Window->Children[ChildNr], NewParentName);

    if (Window->GetParent()==Window->GetRoot()) OutFile << "\n";
}


// Recursively saves the window hierarchy of the passed window and all of its children.
static void SaveWindowHierarchy(std::ostream& OutFile, cf::GuiSys::WindowT* Window, const wxString& ParentName)
{
    if (Window!=Window->GetRoot()) // Root window is saved separately (see Save_cgui).
    {
        wxString HierarchyParent=ParentName;

          if (HierarchyParent=="") HierarchyParent=Window->GetRoot()->Name;
        else                       HierarchyParent=HierarchyParent.substr(0, HierarchyParent.size()-1); // Strip parent of its last '.' character.

        OutFile << HierarchyParent << ":AddChild(" << ParentName+Window->Name << ");\n";
    }

    wxString NewParentName=Window->GetRoot()==Window ? "" : ParentName+Window->Name+".";

    for (unsigned long ChildNr=0; ChildNr<Window->Children.Size(); ChildNr++)
        SaveWindowHierarchy(OutFile, Window->Children[ChildNr], NewParentName);

    if (Window->GetParent()==Window->GetRoot()) OutFile << "\n";
}


// Saves the window initalization method for the root window.
static void SaveRootInitialization(std::ostream& OutFile, cf::GuiSys::WindowT* Root, const GuiPropertiesT& GuiProps)
{
    wxASSERT(Root==Root->GetRoot());

    OutFile << "function " << Root->Name << ":OnInit()\n";

    Root->WriteInitMethod(OutFile);

    OutFile << "\n";
    OutFile << "    gui:activate      (" << (GuiProps.Activate    ? "true" : "false") << ");\n";
    OutFile << "    gui:setInteractive(" << (GuiProps.Interactive ? "true" : "false") << ");\n";
    OutFile << "    gui:showMouse     (" << (GuiProps.ShowMouse   ? "true" : "false") << ");\n";

    if (GuiProps.DefaultFocus!="")
        OutFile << "    gui:setFocus      (" <<  GuiProps.DefaultFocus << ");\n";

    OutFile << "end\n\n";
}


// Recursively saves the window initilization function of the window passed and all of its children.
static void SaveWindowInitialization(std::ostream& OutFile, cf::GuiSys::WindowT* Window, const wxString& ParentName)
{
    if (Window!=Window->GetRoot())
    {
        OutFile << "function " << ParentName+Window->Name << ":OnInit()\n";

        Window->WriteInitMethod(OutFile);

        OutFile << "end\n\n";
    }

    wxString NewParentName=Window->GetRoot()==Window ? "" : ParentName+Window->Name+".";

    for (unsigned long ChildNr=0; ChildNr<Window->Children.Size(); ChildNr++)
        SaveWindowInitialization(OutFile, Window->Children[ChildNr], NewParentName);
}


bool GuiDocumentT::SaveInit_cgui(std::ostream& OutFile)
{
    CheckWindowNames(m_RootWindow);

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

    OutFile << "gui:SetRootWindow(" << m_RootWindow->Name << ");\n";

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
