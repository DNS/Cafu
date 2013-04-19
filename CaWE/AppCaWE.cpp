/*
=================================================================================
This file is part of Cafu, the open-source game engine and graphics engine
for multiplayer, cross-platform, real-time 3D action.
Copyright (C) 2002-2013 Carsten Fuchs Software.

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

#include "wx/wx.h"
#include "wx/confbase.h"
#include "wx/dir.h"
#include "wx/fileconf.h"
#include "wx/filename.h"
#include "wx/image.h"
#include "wx/snglinst.h"
#include "wx/splash.h"
#include "wx/stdpaths.h"

#include "AppCaWE.hpp"
#include "ArtProvider.hpp"
#include "ChildFrameViewWin2D.hpp"
#include "ChildFrameViewWin3D.hpp"
#include "CursorMan.hpp"
#include "GameConfig.hpp"
#include "ParentFrame.hpp"
#include "MapDocument.hpp"
#include "MapElement.hpp"
#include "Options.hpp"
#include "EditorMaterialManager.hpp"
#include "ToolbarMaterials.hpp"
#include "Tool.hpp"
#include "ToolManager.hpp"

#include "ClipSys/CollisionModelMan_impl.hpp"
#include "ConsoleCommands/Console.hpp"
#include "ConsoleCommands/ConsoleStdout.hpp"
#include "ConsoleCommands/ConsoleInterpreter.hpp"
#include "ConsoleCommands/ConsoleInterpreterImpl.hpp"
#include "ConsoleCommands/ConVar.hpp"
#include "ConsoleCommands/ConFunc.hpp"
#include "FileSys/FileManImpl.hpp"
#include "GuiSys/AllComponents.hpp"
#include "GuiSys/Window.hpp"
#include "MaterialSystem/MaterialManagerImpl.hpp"
#include "TypeSys.hpp"

#include <fstream>


static cf::ConsoleStdoutT ConsoleStdout;
cf::ConsoleI* Console=&ConsoleStdout;

//static cf::ConsoleFileT ConsoleFile("info.log");
//cf::ConsoleI* Console=&ConsoleFile;

static cf::FileSys::FileManImplT FileManImpl;
cf::FileSys::FileManI* cf::FileSys::FileMan=&FileManImpl;

static cf::ClipSys::CollModelManImplT CCM;
cf::ClipSys::CollModelManI* cf::ClipSys::CollModelMan=&CCM;

ConsoleInterpreterI* ConsoleInterpreter=NULL;
MaterialManagerI*    MaterialManager   =NULL;


OptionsT Options;


IMPLEMENT_APP(AppCaWE)


AppCaWE::AppCaWE()
    : wxApp(),
      m_Locale(NULL),
      m_CmdLineParser(),
      m_FileConfig(NULL),
      m_ParentFrame(NULL)
{
    static ConsoleInterpreterImplT ConInterpreterImpl;
    ConsoleInterpreter=&ConInterpreterImpl;

    // All global convars and confuncs have registered themselves in linked lists.
    // Register them with the console interpreter now.
    ConFuncT::RegisterStaticList();
    ConVarT ::RegisterStaticList();

    SetAppName("CaWE");
    SetVendorName("Carsten Fuchs Software");
    wxStandardPaths::Get().UseAppInfo(wxStandardPaths::AppInfo_VendorName | wxStandardPaths::AppInfo_AppName);
}


bool AppCaWE::OnInit()
{
    cf::GuiSys::GetComponentTIM().Init();   // The one-time init of the components type info manager.
    cf::GuiSys::GetWindowTIM().Init();      // The one-time init of the Window type info manager.
    GetMapElemTIM().Init();                 // The one-time init of the map elements type info manager.
    GetToolTIM().Init();                    // The one-time init of the tools type info manager.

    // Parse the command line.
    // Note that this replaces (and in fact conflicts with) wxApp::OnInit().
    m_CmdLineParser.SetCmdLine(argc, argv);
    OnInitCmdLine(m_CmdLineParser);
    m_CmdLineParser.AddParam("filename", wxCMD_LINE_VAL_STRING, wxCMD_LINE_PARAM_OPTIONAL | wxCMD_LINE_PARAM_MULTIPLE);

    if (m_CmdLineParser.Parse() != 0)
    {
        OnExit();
        return false;
    }

    OnCmdLineParsed(m_CmdLineParser);   // Just for setting wxLog to verbose when "--verbose" is given.

    #ifndef NDEBUG
    {
        WriteLuaDoxygenHeaders();
    }
    #endif

    const std::string AppDir="./CaWE";

    // This is for registering the CaWE.cmat file farther below.
    cf::FileSys::FileMan->MountFileSystem(cf::FileSys::FS_TYPE_LOCAL_PATH, AppDir+"/res/", AppDir+"/res/");
    cf::FileSys::FileMan->MountFileSystem(cf::FileSys::FS_TYPE_LOCAL_PATH, "./", "");


    for (unsigned int DirNr=0; DirNr<2; DirNr++)
    {
        wxString TestDirs[]={ "CaWE", "Games" };

        if (!wxDirExists(TestDirs[DirNr]))
        {
            wxMessageDialog Msg(NULL, "Could not find directory \"" + TestDirs[DirNr] + "\" in " + wxGetCwd(),
                "Subdirectory not found", wxOK | wxCANCEL | wxCANCEL_DEFAULT | wxICON_ERROR);

            Msg.SetExtendedMessage("Did you run the program from the right working directory?\n\nTo get help, please post this error at the Cafu forums or mailing-list.");
            Msg.SetOKCancelLabels("Open www.cafu.de", "Close");

            while (Msg.ShowModal()==wxID_OK)
                wxLaunchDefaultBrowser("http://www.cafu.de");

            OnExit();
            return false;
        }
    }


    const wxString UserDataDir=wxStandardPaths::Get().GetUserDataDir();

    if (!wxFileName::Mkdir(UserDataDir, 0777, wxPATH_MKDIR_FULL))
        wxMessageBox(wxString("Config file storage path \n")+UserDataDir+"\n doesn't exist, and I was not able to create it, either.", "Warning!");


    static wxSingleInstanceChecker SIC(GetAppName()+"-"+wxGetUserId(), UserDataDir);

    if (SIC.IsAnotherRunning())
    {
        wxMessageBox("An instance of CaWE is already running!");
        OnExit();
        return false;
    }


    // Undo the wx locale initialization, as we want to be sure to use the same (default) locale "C" always and everywhere.
    // Using other locales introduces a lot of subtle errors. E.g. reading floating point numbers from anywhere
    // (like map files!) fails because e.g. "1.4" is no proper floating point string in the German locale (but "1,4" is).
    // setlocale(LC_ALL, "C");      // This alone is not enough, see http://trac.wxwidgets.org/ticket/12970 for details.
    for (int LangNr=wxLANGUAGE_ENGLISH; LangNr<=wxLANGUAGE_ENGLISH_ZIMBABWE; LangNr++)
    {
        if (wxLocale::IsAvailable(LangNr))
        {
            m_Locale=new wxLocale(LangNr, wxLOCALE_DONT_LOAD_DEFAULT);

            wxLogDebug("Program locale set to %s (%s, %s).", m_Locale->GetName(), m_Locale->GetCanonicalName(), m_Locale->GetLocale());
            break;
        }
    }

    if (!m_Locale)
    {
        // If the above for some reason didn't work, set at least the CRT to the "C" locale.
        setlocale(LC_ALL, "C");
        wxLogDebug("Program locale set to \"C\".");
    }


    // Activate a log console as the wx debug target.
    // wxLog::SetActiveTarget(new wxLogWindow(NULL, "wx Debug Console", true, false));

    srand(time(NULL));          // Re-seed the random number generator.
    wxInitAllImageHandlers();   // Needed e.g. for cursor initialization under GTK.

    // Initialize the global cursor manager instance.
    wxASSERT(CursorMan==NULL);
    CursorMan=new CursorManT;

    // Initialize the global art provider.
    if (wxArtProvider::HasNativeProvider())
        wxArtProvider::PushBack(new ArtProviderT("default"));
    else
        wxArtProvider::Push(new ArtProviderT("default"));


    // Set the globally used configuration storage object for easy access via wxConfigBase::Get().
    m_FileConfig=new wxFileConfig(GetAppName(), GetVendorName(), UserDataDir+"/CaWE.config");
    wxConfigBase::Set(m_FileConfig);

    // Setup the global Material Manager pointer.
    static MaterialManagerImplT MatManImpl;

    MaterialManager=&MatManImpl;

    // Register the material script with the CaWE materials definitions.
    if (MaterialManager->RegisterMaterialScript(AppDir+"/res/CaWE.cmat", AppDir+"/res/").Size()==0)
        wxMessageBox("CaWE.cmat not found in \""+AppDir+"\".", "WARNING");

    // Initialize the global options and game configs from the CaWE configuration files.
    // This also registers additional material scripts with the MaterialManager that must be known before the ParentFrameT ctor is called,
    // because under wxMSW, the ParentFrameT ctor calls the ParentFrameT::OnShow() method, where the global Gui Manager initialization
    // expects the "Gui/Default" material to be known.
    Options.Init();


    // Create the MDI parent frame.
    m_ParentFrame=new ParentFrameT(m_CmdLineParser);

    SetTopWindow(m_ParentFrame);


    // Check for autosave files.
    wxDir AutoSaveDir(UserDataDir);

    if (AutoSaveDir.IsOpened())
    {
        ArrayT<wxString> AutoSavedFileNames;
        wxString         AutoSavedFileName;

        for (bool more=AutoSaveDir.GetFirst(&AutoSavedFileName, "autosave*.cmap", wxDIR_FILES); more; more=AutoSaveDir.GetNext(&AutoSavedFileName))
            AutoSavedFileNames.PushBack(AutoSavedFileName);

        if (AutoSavedFileNames.Size()>0)
        {
            const unsigned long ID=wxGetLocalTime();
            wxString RecoveredList;

            for (unsigned long FileNr=0; FileNr<AutoSavedFileNames.Size(); FileNr++)
            {
                const wxString NewName =wxString::Format("recovered_%lu_%lu.cmap", FileNr, ID);
                const bool     RenameOK=wxRenameFile(UserDataDir+"/"+AutoSavedFileNames[FileNr], UserDataDir+"/"+NewName);

                RecoveredList+=RenameOK ? NewName : AutoSavedFileNames[FileNr] + "(WARNING: renaming to "+NewName+" failed! Back it up immediately yourself!)";
                RecoveredList+="\n";
            }

            wxMessageBox(wxString("A previously crashed or otherwise prematurely aborted instance of CaWE left auto-saved files.\n")+
                         "They are kept in "+UserDataDir+" under the name(s)\n\n"+RecoveredList+"\n"+
                         "You may open them as usual, and save, move, delete or rename them at your convenience.",
                         "Crash recovery: auto-save files found and recovered!", wxOK | wxICON_EXCLAMATION);
        }
    }
    else
    {
        wxMessageBox(wxString("Could not open auto-save directory ")+UserDataDir, "WARNING", wxOK | wxICON_ERROR);
    }

    return true;
}


int AppCaWE::OnExit()
{
    // TODO: delete Options; Options=NULL;

    wxConfigBase::Set(NULL);

    delete m_FileConfig;
    m_FileConfig=NULL;

    ConsoleInterpreter=NULL;     // This is very important, to make sure that no ConFuncT or ConVarT dtor accesses the ConsoleInterpreter that might already have been destroyed then.

    // Shutdown the global cursor manager instance.
    delete CursorMan;
    CursorMan=NULL;

    delete m_Locale;
    m_Locale=NULL;

    return wxApp::OnExit();
}


extern "C"
{
    #include <lua.h>
    #include <lualib.h>
    #include <lauxlib.h>
}


namespace
{
    class VarVisitorGetLuaTypeStringT : public cf::TypeSys::VisitorConstT
    {
        public:

        VarVisitorGetLuaTypeStringT() { }

        const std::string& GetCppType() const { return m_CppType; }
        const std::string& GetLuaType() const { return m_LuaType; }
        const std::string& GetChoices() const { return m_Choices; }

        void visit(const cf::TypeSys::VarT<float>& Var);
        void visit(const cf::TypeSys::VarT<double>& Var);
        void visit(const cf::TypeSys::VarT<int>& Var);
        void visit(const cf::TypeSys::VarT<unsigned int>& Var);
        void visit(const cf::TypeSys::VarT<bool>& Var);
        void visit(const cf::TypeSys::VarT<std::string>& Var);
        void visit(const cf::TypeSys::VarT<Vector2fT>& Var);
        void visit(const cf::TypeSys::VarT<Vector3fT>& Var);
        void visit(const cf::TypeSys::VarT< ArrayT<std::string> >& Var);


        private:

        template<class T> static std::string GetChoicesString(const cf::TypeSys::VarT<T>& Var);

        std::string m_CppType;
        std::string m_LuaType;
        std::string m_Choices;
    };


    template<class T> std::string VarVisitorGetLuaTypeStringT::GetChoicesString(const cf::TypeSys::VarT<T>& Var)
    {
        std::ostringstream  out;
        ArrayT<std::string> Strings;
        ArrayT<T>           Values;

        Var.GetChoices(Strings, Values);

        for (unsigned int i = 0; i < Strings.Size(); i++)
        {
            out << "<tr>";
            out << "<td>" << Values[i]  << "</td>";
            out << "<td>" << Strings[i] << "</td>";
            out << "</tr>";
            if (i+1 < Strings.Size()) out << "\n";
        }

        return out.str();
    }


    template<> std::string VarVisitorGetLuaTypeStringT::GetChoicesString(const cf::TypeSys::VarT< ArrayT<std::string> >& Var)
    {
        // We have no operator << that could take entire ArrayT<std::string> as input,
        // so just specialize in order to ignore this case at this time.
        return "";
    }


    void VarVisitorGetLuaTypeStringT::visit(const cf::TypeSys::VarT<float>& Var)                 { m_CppType = "float";               m_LuaType = "number";  m_Choices = GetChoicesString(Var); }
    void VarVisitorGetLuaTypeStringT::visit(const cf::TypeSys::VarT<double>& Var)                { m_CppType = "double";              m_LuaType = "number";  m_Choices = GetChoicesString(Var); }
    void VarVisitorGetLuaTypeStringT::visit(const cf::TypeSys::VarT<int>& Var)                   { m_CppType = "int";                 m_LuaType = "number";  m_Choices = GetChoicesString(Var); }
    void VarVisitorGetLuaTypeStringT::visit(const cf::TypeSys::VarT<unsigned int>& Var)          { m_CppType = "unsigned int";        m_LuaType = "number";  m_Choices = GetChoicesString(Var); }
    void VarVisitorGetLuaTypeStringT::visit(const cf::TypeSys::VarT<bool>& Var)                  { m_CppType = "bool";                m_LuaType = "boolean"; m_Choices = GetChoicesString(Var); }
    void VarVisitorGetLuaTypeStringT::visit(const cf::TypeSys::VarT<std::string>& Var)           { m_CppType = "std::string";         m_LuaType = "string";  m_Choices = GetChoicesString(Var); }
    void VarVisitorGetLuaTypeStringT::visit(const cf::TypeSys::VarT<Vector2fT>& Var)             { m_CppType = "Vector2fT";           m_LuaType = "tuple";   m_Choices = GetChoicesString(Var); }
    void VarVisitorGetLuaTypeStringT::visit(const cf::TypeSys::VarT<Vector3fT>& Var)             { m_CppType = "Vector3fT";           m_LuaType = "tuple";   m_Choices = GetChoicesString(Var); }
    void VarVisitorGetLuaTypeStringT::visit(const cf::TypeSys::VarT< ArrayT<std::string> >& Var) { m_CppType = "ArrayT<std::string>"; m_LuaType = "table";   m_Choices = GetChoicesString(Var); }


    wxString FormatDoxyComment(const char* s, const char* Indent)
    {
        if (s && s[0])
        {
            // std::string d = DocMethods[MethodNr].Doc;
            // d = cf::String::Replace(d, "\n", std::string("\n") + Indent + "/// ");

            wxString d = s;
            d.Replace("\n", wxString("\n") + Indent + "/// ");

            // First line must be indented and commented, too.
            d = wxString(Indent) + "/// " + d;

            // Last line must be followed by newline.
            d = d + "\n";

            // Remove trailing whitespace.
            d.Replace(" \n", "\n");

            return d;
        }

        return "";
    }


    void WriteDoxyMethods(std::ofstream& Out, const cf::TypeSys::TypeInfoT* TI)
    {
        if (!TI->MethodsList) return;

        const cf::TypeSys::MethsDocT* DocMethods = TI->DocMethods;

        Out << "    public:\n";

        for (unsigned int MethodNr = 0; TI->MethodsList[MethodNr].name; MethodNr++)
        {
            const char* MethName = TI->MethodsList[MethodNr].name;

            if (strncmp(MethName, "__", 2) == 0) continue;

            Out << "\n";

            if (!DocMethods)
            {
                Out << "    // WARNING: No method documentation strings for this class!\n";
                Out << "    " << MethName << "();\n";

                continue;
            }

            if (!DocMethods[MethodNr].Name)
            {
                Out << "    // WARNING: Unexpected end of method documentation strings encountered!\n";
                Out << "    " << MethName << "();\n";

                DocMethods = NULL;
                continue;
            }

            if (strcmp(MethName, DocMethods[MethodNr].Name) != 0)
            {
                Out << "    // WARNING: Mismatching method documentation strings encountered!\n";
                Out << "    " << MethName << "();\n";

                DocMethods = NULL;
                continue;
            }

            Out << FormatDoxyComment(DocMethods[MethodNr].Doc, "    ");
            Out << "    ";
            if (DocMethods[MethodNr].ReturnType && DocMethods[MethodNr].ReturnType[0]) Out << DocMethods[MethodNr].ReturnType << " ";
            Out << MethName;
            if (DocMethods[MethodNr].Parameters && DocMethods[MethodNr].Parameters[0])
            {
                Out << DocMethods[MethodNr].Parameters;
            }
            else
            {
                Out << "()";
            }
            Out << ";\n";
        }
    }


    void WriteDoxyVars(std::ofstream& Out, const cf::TypeSys::VarManT& VarMan, const cf::TypeSys::TypeInfoT* TI=NULL)
    {
        const ArrayT<cf::TypeSys::VarBaseT*>& Vars    = VarMan.GetArray();
        const cf::TypeSys::VarsDocT*          DocVars = TI->DocVars;

        // Note that the Vars are the driving data structure, not the DocVars.
        if (Vars.Size() == 0) return;

        VarVisitorGetLuaTypeStringT Visitor;

        if (TI->MethodsList) Out << "\n\n";
        Out << "    public:\n";

        for (unsigned int VarNr = 0; VarNr < Vars.Size(); VarNr++)
        {
            const cf::TypeSys::VarBaseT& Var = *Vars[VarNr];

            Var.accept(Visitor);

            Out << "\n";

            if (DocVars && DocVars[VarNr].Name && strcmp(Var.GetName(), DocVars[VarNr].Name) == 0)
            {
                Out << FormatDoxyComment(DocVars[VarNr].Doc, "    ");
            }
            else
            {
                Out << "    // WARNING: No, mismatching or incomplete variables documentation strings for this class!\n";
                DocVars = NULL;
            }

            if (Visitor.GetChoices() != "")
            {
                Out << "    ///\n";
                Out << "    /// @par Typical values:\n";
                Out << "    /// <table>\n";
                Out << "    /// <tr><th>Value</th><th>Description</th></tr>\n";
                Out << FormatDoxyComment(Visitor.GetChoices().c_str(), "    ");
                Out << "    /// </table>\n";
                Out << "    ///\n";
            }

            Out << "    " << "/// @cppType{" << Visitor.GetCppType() << "}\n";
            Out << "    " << Visitor.GetLuaType() << " " << Var.GetName() << ";\n";
        }
    }
}


/// This is an auxiliary method for creating Lua scripting documentation for the registered classes.
/// Assuming that the classes registered with this type info manager provide methods for access from Lua scripts,
/// this method creates Doxygen input files ("fake headers") that documentation writers can complete to create
/// related reference documentation.
void AppCaWE::WriteLuaDoxygenHeaders() const
{
    // Write template file for the cf::GuiSyS::WindowT hierarchy.
    {
        std::ofstream Out("Doxygen/scripting/tmpl/GuiWindows.hpp");

        if (Out.is_open())
        {
            Out << "namespace GUI\n";
            Out << "{\n";

            const ArrayT<const cf::TypeSys::TypeInfoT*>& TIs = cf::GuiSys::GetWindowTIM().GetTypeInfosByName();

            for (unsigned int TypeNr = 0; TypeNr < TIs.Size(); TypeNr++)
            {
                const cf::TypeSys::TypeInfoT* TI = TIs[TypeNr];

                Out << "\n\n";
                Out << FormatDoxyComment(TI->DocClass, "");

                const std::string InfoNew = std::string(
                    "\n"
                    "If you would like to create a new window explicitly "
                    "(those defined in the CaWE %GUI Editor are instantiated automatically),"   // Don't auto-link "GUI".
                    "use GuiT::new():\n"
                    "\\code{.lua}\n"
                    "    local win = gui:new(\"") + TI->ClassName + std::string("\", \"my_window\")\n"
                    "\\endcode\n");

                if (!TI->Child)   // Only do this for "leaf" classes.
                    Out << FormatDoxyComment(InfoNew.c_str(), "");

                Out << "/// @cppName{" << TI->ClassName << "}\n";
                Out << "class " << TI->ClassName;
                if (TI->Base) Out << " : public " << TI->BaseClassName;
                Out << "\n";
                Out << "{\n";

                WriteDoxyMethods(Out, TI);

                Out << "};\n";
            }

            Out << "\n";
            Out << "\n";
            Out << "}   // namespace GUI\n";
        }
    }

    // Write template file for the cf::GuiSyS::ComponentBaseT hierarchy.
    {
        std::ofstream Out("Doxygen/scripting/tmpl/GuiComponents.hpp");

        if (Out.is_open())
        {
            Out << "namespace GUI\n";
            Out << "{\n";

            const ArrayT<const cf::TypeSys::TypeInfoT*>& TIs = cf::GuiSys::GetComponentTIM().GetTypeInfosByName();

            for (unsigned int TypeNr = 0; TypeNr < TIs.Size(); TypeNr++)
            {
                const cf::TypeSys::TypeInfoT* TI = TIs[TypeNr];

                // Skip the CaWE implementation-specific component.
                if (strcmp(TI->ClassName, "ComponentSelectionT") == 0)
                    continue;

                Out << "\n\n";
                Out << FormatDoxyComment(TI->DocClass, "");

                if (TI->DocVars)
                {
                    Out << FormatDoxyComment("\n"
                        "Note that the variables of this class (also referred to as \"Public Attributes\" or \"Member Data\")\n"
                        "must be used with the get() and set() methods at this time -- see get() and set() for details.", "");
                }

                const std::string InfoNew = std::string(
                    "\n"
                    "If you would like to create a new component of this type explicitly "
                    "(those defined in the CaWE %GUI Editor are instantiated automatically),"   // Don't auto-link "GUI".
                    "use GuiT::new():\n"
                    "\\code{.lua}\n"
                    "    local comp = gui:new(\"") + TI->ClassName + std::string("\")\n"
                    "\\endcode\n");

                if (!TI->Child)   // Only do this for "leaf" classes.
                    Out << FormatDoxyComment(InfoNew.c_str(), "");

                Out << "/// @cppName{" << TI->ClassName << "}\n";
                Out << "class " << TI->ClassName;
                if (TI->Base) Out << " : public " << TI->BaseClassName;
                Out << "\n";
                Out << "{\n";

                WriteDoxyMethods(Out, TI);

                // Write variables.
                IntrusivePtrT<cf::GuiSys::ComponentBaseT> Comp = static_cast<cf::GuiSys::ComponentBaseT*>(
                    TI->CreateInstance(
                        cf::TypeSys::CreateParamsT()));

                cf::TypeSys::VarManT& VarMan = Comp->GetMemberVars();

                WriteDoxyVars(Out, VarMan, TI);

                Out << "};\n"; Out.flush();
            }

            Out << "\n";
            Out << "\n";
            Out << "}   // namespace GUI\n";
        }
    }
}
