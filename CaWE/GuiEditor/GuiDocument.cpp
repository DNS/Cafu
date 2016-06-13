/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "GuiDocument.hpp"
#include "../GameConfig.hpp"
#include "../EditorMaterialEngine.hpp"

#include "GuiSys/CompBase.hpp"
#include "String.hpp"
#include "TypeSys.hpp"
#include "VarVisitorsLua.hpp"

#include <fstream>


using namespace GuiEditor;


GuiPropertiesT::GuiPropertiesT(cf::GuiSys::GuiImplT& Gui)
    : Activate(Gui.GetIsActive()),
      Interactive(Gui.GetIsInteractive()),
      ShowMouse(Gui.IsMouseShown()),
      DefaultFocus(Gui.GetFocusWindow()!=NULL ? Gui.GetFocusWindow()->GetBasics()->GetWindowName() : "")
{
}


GuiDocumentT::GuiDocumentT(GameConfigT* GameConfig, const wxString& GuiInitFileName)
    : m_ScriptState(),
      m_Gui(NULL),
      m_Selection(),
      m_EditorMaterials(),
      m_GameConfig(GameConfig),
      m_DocAdapter(*this)
{
    cf::GuiSys::GuiImplT::InitScriptState(m_ScriptState);

    m_Gui = new cf::GuiSys::GuiImplT(m_ScriptState, GameConfig->GetGuiResources());

    // Load GUI initialization script and create documents window hierarchy.
    if (GuiInitFileName!="")
    {
        m_Gui->LoadScript(
            std::string(GuiInitFileName),
            cf::GuiSys::GuiImplT::InitFlag_InGuiEditor);
    }
    else
    {
        m_Gui->LoadScript(
            "local gui = ...\n"
            "local Root = gui:new('WindowT', 'Root')\n"
            "gui:SetRootWindow(Root)\n"
            "\n"
            "function Root:OnInit()\n"
            "    self:GetTransform():set('Pos', 0, 0)\n"
            "    self:GetTransform():set('Size', 640, 480)\n"
            "\n"
            "    gui:activate      (true)\n"
            "    gui:setInteractive(true)\n"
            "    gui:showMouse     (false)\n"
            "    gui:setFocus      (Root)\n"
            "end\n",
            cf::GuiSys::GuiImplT::InitFlag_InlineCode | cf::GuiSys::GuiImplT::InitFlag_InGuiEditor);
    }

    m_GuiProperties = GuiPropertiesT(*m_Gui);

    // Set the root window as the initial selection.
    ArrayT< IntrusivePtrT<cf::GuiSys::WindowT> > NewSel;

    NewSel.PushBack(GetRootWindow());
    SetSelection(NewSel);

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
}


void GuiDocumentT::SetSelection(const ArrayT< IntrusivePtrT<cf::GuiSys::WindowT> >& NewSelection)
{
    // Clear the previous selection.
    for (unsigned long SelNr=0; SelNr<m_Selection.Size(); SelNr++)
        GetSelComp(m_Selection[SelNr])->SetSelected(false);

    m_Selection.Clear();

    for (unsigned long NewSelNr=0; NewSelNr<NewSelection.Size(); NewSelNr++)
    {
        m_Selection.PushBack(NewSelection[NewSelNr]);

        GetSelComp(NewSelection[NewSelNr])->SetSelected(true);
    }
}


namespace
{
    const char* StripNamespace(const char* ClassName)
    {
        if (strncmp(ClassName, "GuiSys::", 8) == 0)
            return ClassName + 8;

        return ClassName;
    }
}


// Recursively saves the window instantiation of the passed window and all of its children.
static void SaveWindowInstantiation(std::ostream& OutFile, IntrusivePtrT<cf::GuiSys::WindowT> Window, const wxString& ParentName)
{
    if (ParentName == "")
    {
        // We must not modify the global script state.
        OutFile << "local ";
    }

    OutFile << ParentName << Window->GetBasics()->GetWindowName() << " = gui:new(\"" << StripNamespace(Window->GetType()->ClassName) << "\", \"" << Window->GetBasics()->GetWindowName() << "\")\n";

    const wxString NewParentName = ParentName + Window->GetBasics()->GetWindowName() + ".";

    for (unsigned long ChildNr = 0; ChildNr < Window->GetChildren().Size(); ChildNr++)
        SaveWindowInstantiation(OutFile, Window->GetChildren()[ChildNr], NewParentName);
}


// Recursively saves the window hierarchy of the passed window and all of its children.
static void SaveWindowHierarchy(std::ostream& OutFile, IntrusivePtrT<cf::GuiSys::WindowT> Window, const wxString& ParentName)
{
    if (ParentName != "")
        OutFile << ParentName << ":AddChild(" << ParentName << "." << Window->GetBasics()->GetWindowName() << ")\n";

    const wxString NewParentName = (ParentName != "" ? ParentName + "." : "") + Window->GetBasics()->GetWindowName();

    for (unsigned long ChildNr = 0; ChildNr < Window->GetChildren().Size(); ChildNr++)
        SaveWindowHierarchy(OutFile, Window->GetChildren()[ChildNr], NewParentName);
}


static void SaveComponents(std::ostream& OutFile, IntrusivePtrT<cf::GuiSys::WindowT> Window)
{
    cf::TypeSys::VarVisitorToLuaCodeT ToLua(OutFile);

    if (!Window->GetBasics()->IsShown())
        OutFile << "    self:GetBasics():set(\"Show\", false)\n";

    const ArrayT<cf::TypeSys::VarBaseT*>& TrafoVars = Window->GetTransform()->GetMemberVars().GetArray();

    for (unsigned int VarNr = 0; VarNr < TrafoVars.Size(); VarNr++)
    {
        OutFile << "    self:GetTransform():set(\"" << TrafoVars[VarNr]->GetName() << "\", ";
        TrafoVars[VarNr]->accept(ToLua);
        OutFile << ")\n";
    }

    if (Window->GetComponents().Size() == 0)
        return;

    for (unsigned int CompNr = 1; CompNr <= Window->GetComponents().Size(); CompNr++)
    {
        IntrusivePtrT<cf::GuiSys::ComponentBaseT> Comp = Window->GetComponents()[CompNr - 1];
        const ArrayT<cf::TypeSys::VarBaseT*>&     Vars = Comp->GetMemberVars().GetArray();

        OutFile << "\n";
        OutFile << "    local c" << CompNr << " = gui:new(\"" << StripNamespace(Comp->GetType()->ClassName) << "\")\n";

        for (unsigned int VarNr = 0; VarNr < Vars.Size(); VarNr++)
        {
            const cf::TypeSys::VarBaseT* Var = Vars[VarNr];

            // This is the same test as in WriteDoxyVars() in AppCaWE.cpp, see there for details.
            wxASSERT(Var->GetName() == cf::String::ToLuaIdentifier(Var->GetName()));

            OutFile << "    c" << CompNr << ":set(\"" << Var->GetName() << "\", ";
            Var->accept(ToLua);
            OutFile << ")\n";
        }
    }

    OutFile << "\n";
    OutFile << "    self:AddComponent(";
    for (unsigned int CompNr = 1; CompNr <= Window->GetComponents().Size(); CompNr++)
    {
        OutFile << "c" << CompNr;
        if (CompNr < Window->GetComponents().Size()) OutFile << ", ";
    }
    OutFile << ")\n";
}


// Recursively saves the window initialization function of the window passed and all of its children.
static void SaveWindowInitialization(std::ostream& OutFile, IntrusivePtrT<cf::GuiSys::WindowT> Window, const wxString& ParentName)
{
    OutFile << "\nfunction " << ParentName + Window->GetBasics()->GetWindowName() << ":OnInit()\n";

    SaveComponents(OutFile, Window);

    OutFile << "end\n";

    const wxString NewParentName = ParentName + Window->GetBasics()->GetWindowName() + ".";

    for (unsigned long ChildNr = 0; ChildNr < Window->GetChildren().Size(); ChildNr++)
        SaveWindowInitialization(OutFile, Window->GetChildren()[ChildNr], NewParentName);
}


bool GuiDocumentT::SaveInit_cgui(std::ostream& OutFile)
{
    OutFile << "-- This is a Cafu engine GUI script file, written by CaWE, the Cafu World Editor.\n";
    OutFile << "-- You CAN edit this file manually, but note that CaWE may overwrite your changes.\n";
    OutFile << "-- It is recommended that you place all your customizations like method overrides\n";
    OutFile << "-- and effects into a separate cgui file that calls loadfile() for including this.\n";
    OutFile << "\n\n";
    OutFile << "-- Instantiation of all windows.\n";
    OutFile << "-- *****************************\n";
    OutFile << "\n";
    OutFile << "local gui = ...\n";

    SaveWindowInstantiation(OutFile, GetRootWindow(), "");

    OutFile << "\n\n";
    OutFile << "-- Set the GUIs root window.\n";
    OutFile << "-- *************************\n";
    OutFile << "\n";
    OutFile << "gui:SetRootWindow("  << GetRootWindow()->GetBasics()->GetWindowName() << ")\n";
    OutFile << "gui:activate("       << (m_GuiProperties.Activate    ? "true" : "false") << ")\n";
    OutFile << "gui:setInteractive(" << (m_GuiProperties.Interactive ? "true" : "false") << ")\n";
    OutFile << "gui:showMouse("      << (m_GuiProperties.ShowMouse   ? "true" : "false") << ")\n";

    if (m_GuiProperties.DefaultFocus != "")
    {
        IntrusivePtrT<cf::GuiSys::WindowT> FocusWin = GetRootWindow()->Find(std::string(m_GuiProperties.DefaultFocus));

        if (FocusWin != NULL)
        {
            std::string FocusName = FocusWin->GetBasics()->GetWindowName();

            while (true)
            {
                FocusWin = FocusWin->GetParent();
                if (FocusWin == NULL) break;
                FocusName = FocusWin->GetBasics()->GetWindowName() + "." + FocusName;
            }

            OutFile << "gui:setFocus(" << FocusName << ")\n";
        }
    }

    OutFile << "\n\n";
    OutFile << "-- Setup the window hierarchy.\n";
    OutFile << "-- ***************************\n";
    OutFile << "\n";

    SaveWindowHierarchy(OutFile, GetRootWindow(), "");

    OutFile << "\n\n";
    OutFile << "-- Initialization of the window contents (\"constructors\").\n";
    OutFile << "-- *******************************************************\n";

    SaveWindowInitialization(OutFile, GetRootWindow(), "");

    if (OutFile.fail())
    {
        wxMessageBox("There was a file error.", "Message", wxOK | wxICON_EXCLAMATION);
        return false;
    }

    return true;
}


/*static*/ IntrusivePtrT<ComponentSelectionT> GuiDocumentT::GetSelComp(IntrusivePtrT<cf::GuiSys::WindowT> Win)
{
    IntrusivePtrT<ComponentSelectionT> SelComp = dynamic_pointer_cast<ComponentSelectionT>(Win->GetApp());

    if (SelComp.IsNull())
    {
        SelComp = new ComponentSelectionT();

        Win->SetApp(SelComp);
        wxASSERT(!dynamic_pointer_cast<ComponentSelectionT>(Win->GetApp()).IsNull());
    }

    return SelComp;
}
