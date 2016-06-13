/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
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
#include "CursorMan.hpp"
#include "GameConfig.hpp"
#include "ParentFrame.hpp"
#include "Options.hpp"
#include "EditorMaterialManager.hpp"
#include "MapEditor/MapElement.hpp"
#include "MapEditor/Tool.hpp"

#include "ClipSys/CollisionModelMan_impl.hpp"
#include "ConsoleCommands/Console.hpp"
#include "ConsoleCommands/ConsoleStdout.hpp"
#include "ConsoleCommands/ConsoleInterpreter.hpp"
#include "ConsoleCommands/ConsoleInterpreterImpl.hpp"
#include "ConsoleCommands/ConVar.hpp"
#include "ConsoleCommands/ConFunc.hpp"
#include "FileSys/FileManImpl.hpp"
#include "GameSys/AllComponents.hpp"
#include "GameSys/Entity.hpp"
#include "GameSys/World.hpp"
#include "GuiSys/AllComponents.hpp"
#include "GuiSys/GuiImpl.hpp"
#include "GuiSys/Window.hpp"
#include "MaterialSystem/MaterialManagerImpl.hpp"
#include "SoundSystem/SoundShaderManagerImpl.hpp"
#include "SoundSystem/SoundSys.hpp"
#include "String.hpp"
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

static SoundShaderManagerImplT s_SSM;
SoundShaderManagerI* SoundShaderManager = &s_SSM;

SoundSysI* SoundSystem = NULL;


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
    cf::GameSys::GetComponentTIM().Init();      // The one-time init of the GameSys components type info manager.
    cf::GameSys::GetGameSysEntityTIM().Init();  // The one-time init of the GameSys entity type info manager.
    cf::GameSys::GetWorldTIM().Init();          // The one-time init of the GameSys world type info manager.

    cf::GuiSys::GetComponentTIM().Init();       // The one-time init of the GuiSys components type info manager.
    cf::GuiSys::GetWindowTIM().Init();          // The one-time init of the GuiSys window type info manager.
    cf::GuiSys::GetGuiTIM().Init();             // The one-time init of the GuiSys GUI type info manager.

    GetMapElemTIM().Init();                     // The one-time init of the map elements type info manager.
    GetToolTIM().Init();                        // The one-time init of the tools type info manager.

    // Parse the command line.
    // Note that this replaces (and in fact conflicts with) wxApp::OnInit().
    m_CmdLineParser.SetCmdLine(argc, argv);
    OnInitCmdLine(m_CmdLineParser);
    m_CmdLineParser.AddParam("filename", wxCMD_LINE_VAL_STRING, wxCMD_LINE_PARAM_OPTIONAL | wxCMD_LINE_PARAM_MULTIPLE);
    m_CmdLineParser.AddSwitch("d", "update-doxygen", "Update the scripting documentation templates, then quit.");
    m_CmdLineParser.SetSwitchChars("-");

    if (m_CmdLineParser.Parse() != 0)
    {
        OnExit();
        return false;
    }

    OnCmdLineParsed(m_CmdLineParser);   // Just for setting wxLog to verbose when "--verbose" is given.

    if (m_CmdLineParser.Found("update-doxygen"))
    {
        WriteLuaDoxygenHeaders();

        OnExit();
        return false;
    }

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
        void visit(const cf::TypeSys::VarT<uint16_t>& Var);
        void visit(const cf::TypeSys::VarT<uint8_t>& Var);
        void visit(const cf::TypeSys::VarT<bool>& Var);
        void visit(const cf::TypeSys::VarT<std::string>& Var);
        void visit(const cf::TypeSys::VarT<Vector2fT>& Var);
        void visit(const cf::TypeSys::VarT<Vector3fT>& Var);
        void visit(const cf::TypeSys::VarT<Vector3dT>& Var);
        void visit(const cf::TypeSys::VarT<BoundingBox3dT>& Var);
        void visit(const cf::TypeSys::VarArrayT<uint32_t>& Var);
        void visit(const cf::TypeSys::VarArrayT<uint16_t>& Var);
        void visit(const cf::TypeSys::VarArrayT<uint8_t>& Var);
        void visit(const cf::TypeSys::VarArrayT<std::string>& Var);


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


    template<> std::string VarVisitorGetLuaTypeStringT::GetChoicesString(const cf::TypeSys::VarT<BoundingBox3dT>& Var)
    {
        // We have no operator << that could take entire BoundingBox3dT as input,
        // so just specialize in order to ignore this case at this time.
        return "";
    }


    void VarVisitorGetLuaTypeStringT::visit(const cf::TypeSys::VarT<float>& Var)            { m_CppType = "float";               m_LuaType = "number";  m_Choices = GetChoicesString(Var); }
    void VarVisitorGetLuaTypeStringT::visit(const cf::TypeSys::VarT<double>& Var)           { m_CppType = "double";              m_LuaType = "number";  m_Choices = GetChoicesString(Var); }
    void VarVisitorGetLuaTypeStringT::visit(const cf::TypeSys::VarT<int>& Var)              { m_CppType = "int";                 m_LuaType = "number";  m_Choices = GetChoicesString(Var); }
    void VarVisitorGetLuaTypeStringT::visit(const cf::TypeSys::VarT<unsigned int>& Var)     { m_CppType = "unsigned int";        m_LuaType = "number";  m_Choices = GetChoicesString(Var); }
    void VarVisitorGetLuaTypeStringT::visit(const cf::TypeSys::VarT<uint16_t>& Var)         { m_CppType = "uint16_t";            m_LuaType = "number";  m_Choices = GetChoicesString(Var); }
    void VarVisitorGetLuaTypeStringT::visit(const cf::TypeSys::VarT<uint8_t>& Var)          { m_CppType = "uint8_t";             m_LuaType = "number";  m_Choices = GetChoicesString(Var); }
    void VarVisitorGetLuaTypeStringT::visit(const cf::TypeSys::VarT<bool>& Var)             { m_CppType = "bool";                m_LuaType = "boolean"; m_Choices = GetChoicesString(Var); }
    void VarVisitorGetLuaTypeStringT::visit(const cf::TypeSys::VarT<std::string>& Var)      { m_CppType = "std::string";         m_LuaType = "string";  m_Choices = GetChoicesString(Var); }
    void VarVisitorGetLuaTypeStringT::visit(const cf::TypeSys::VarT<Vector2fT>& Var)        { m_CppType = "Vector2fT";           m_LuaType = "tuple";   m_Choices = GetChoicesString(Var); }
    void VarVisitorGetLuaTypeStringT::visit(const cf::TypeSys::VarT<Vector3fT>& Var)        { m_CppType = "Vector3fT";           m_LuaType = "tuple";   m_Choices = GetChoicesString(Var); }
    void VarVisitorGetLuaTypeStringT::visit(const cf::TypeSys::VarT<Vector3dT>& Var)        { m_CppType = "Vector3dT";           m_LuaType = "tuple";   m_Choices = GetChoicesString(Var); }
    void VarVisitorGetLuaTypeStringT::visit(const cf::TypeSys::VarT<BoundingBox3dT>& Var)   { m_CppType = "BoundingBox3dT";      m_LuaType = "tuple";   m_Choices = GetChoicesString(Var); }
    void VarVisitorGetLuaTypeStringT::visit(const cf::TypeSys::VarArrayT<uint32_t>& Var)    { m_CppType = "ArrayT<uint32_t>";    m_LuaType = "table";   m_Choices = ""; }
    void VarVisitorGetLuaTypeStringT::visit(const cf::TypeSys::VarArrayT<uint16_t>& Var)    { m_CppType = "ArrayT<uint16_t>";    m_LuaType = "table";   m_Choices = ""; }
    void VarVisitorGetLuaTypeStringT::visit(const cf::TypeSys::VarArrayT<uint8_t>& Var)     { m_CppType = "ArrayT<uint8_t>";     m_LuaType = "table";   m_Choices = ""; }
    void VarVisitorGetLuaTypeStringT::visit(const cf::TypeSys::VarArrayT<std::string>& Var) { m_CppType = "ArrayT<std::string>"; m_LuaType = "table";   m_Choices = ""; }


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

        for (unsigned int MethodNr = 0, DocNr = 0; TI->MethodsList[MethodNr].name; MethodNr++)
        {
            const char* MethName = TI->MethodsList[MethodNr].name;

            if (strncmp(MethName, "__", 2) == 0) continue;

            if (!DocMethods)
            {
                Out << "\n";
                Out << "    // WARNING: No method documentation strings for this class!\n";
                Out << "    " << MethName << "();\n";

                continue;
            }

            if (!DocMethods[DocNr].Name)
            {
                Out << "\n";
                Out << "    // WARNING: Unexpected end of method documentation strings encountered!\n";
                Out << "    " << MethName << "();\n";

                DocMethods = NULL;
                continue;
            }

            if (strcmp(MethName, DocMethods[DocNr].Name) != 0)
            {
                Out << "\n";
                Out << "    // WARNING: Mismatching method documentation strings encountered!\n";
                Out << "    " << MethName << "();\n";

                DocMethods = NULL;
                continue;
            }

            // Write the documentation of the method, and all of its overloads.
            while (DocMethods[DocNr].Name != NULL && strcmp(MethName, DocMethods[DocNr].Name) == 0)
            {
                Out << "\n";
                Out << FormatDoxyComment(DocMethods[DocNr].Doc, "    ");
                Out << "    ";
                if (DocMethods[DocNr].ReturnType && DocMethods[DocNr].ReturnType[0]) Out << DocMethods[DocNr].ReturnType << " ";
                Out << MethName;
                if (DocMethods[DocNr].Parameters && DocMethods[DocNr].Parameters[0])
                {
                    Out << DocMethods[DocNr].Parameters;
                }
                else
                {
                    Out << "()";
                }
                Out << ";\n";

                DocNr++;
            }
        }
    }


    void WriteDoxyCallbacks(std::ofstream& Out, const cf::TypeSys::TypeInfoT* TI)
    {
        if (!TI->DocCallbacks) return;

        if (TI->MethodsList) Out << "\n\n";

        // We group all callbacks in a named Doxygen group.
        // Good news is that if the group remains empty for whatever reasons,
        // Doxygen will not generate any output for it at all.
        Out << "    public:\n";
        Out << "\n";
        Out << "    /** @name Event Handlers (Callbacks)\n";
        Out << "     *\n";
        Out << "     * See the \\ref eventhandlers overview page for additional information about the methods in this group.\n";
        Out << "     *\n";
        Out << "     * @{\n";
        Out << "     */\n";

        for (unsigned int Nr = 0; TI->DocCallbacks[Nr].Name; Nr++)
        {
            const cf::TypeSys::MethsDocT& DocCallback = TI->DocCallbacks[Nr];

            Out << "\n";
            Out << FormatDoxyComment(DocCallback.Doc, "    ");
            Out << "    ";
            if (DocCallback.ReturnType && DocCallback.ReturnType[0]) Out << DocCallback.ReturnType << " ";
            Out << DocCallback.Name;
            if (DocCallback.Parameters && DocCallback.Parameters[0])
            {
                Out << DocCallback.Parameters;
            }
            else
            {
                Out << "()";
            }
            Out << ";\n";
        }

        Out << "\n";
        Out << "    /** @} */\n";
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

            if (Var.GetName() != cf::String::ToLuaIdentifier(Var.GetName()))
            {
                // This is relevant because member names such as "Sel. Mode":
                //   - will below generate output like `number Sel. Mode;`, which is not valid C++
                //     (pseudo-)code so that Doxygen cannot produce the intended, proper documentation,
                //   - can be used in Lua code such as `someObject:get("Sel. Mode")`, but achieving
                //     the same by rewriting this as `someObject.Sel. Mode` will not possible.
                Out << "    // WARNING: \"" << Var.GetName() << "\" is not a valid Lua identifier!\n";
            }

            Out << "    " << "/// @cppType{" << Visitor.GetCppType() << "}\n";
            Out << "    " << Visitor.GetLuaType() << " " << Var.GetName() << ";\n";
        }
    }


    // Write a template file for the classes kept in the given TIM.
    void WriteClassHierarchy(const char* FileName, cf::TypeSys::TypeInfoManT& TIM)
    {
        std::ofstream Out(FileName);

        if (!Out.is_open()) return;

        const ArrayT<const cf::TypeSys::TypeInfoT*>& TIs = TIM.GetTypeInfosByName();
        std::string CurrentNamespace = "";

        for (unsigned int TypeNr = 0; TypeNr < TIs.Size(); TypeNr++)
        {
            const cf::TypeSys::TypeInfoT* TI = TIs[TypeNr];

            std::string Namespace = "";
            std::string ClassName = TI->ClassName;

            const size_t ColonPos = ClassName.find("::");
            if (ColonPos != std::string::npos)
            {
                Namespace = ClassName.substr(0, ColonPos);
                ClassName = ClassName.substr(ColonPos + 2);
            }

            // Skip the Gui Editor's "app" component.
            if (Namespace == "GuiEditor")
                continue;

            if (CurrentNamespace != Namespace)
            {
                if (CurrentNamespace != "")
                {
                    Out << "\n";
                    Out << "\n";
                    Out << "}   // namespace " << CurrentNamespace << "\n";
                    Out << "\n";
                }

                CurrentNamespace = Namespace;

                Out << "namespace " << CurrentNamespace << "\n";
                Out << "{\n";
            }

            Out << "\n\n";
            Out << FormatDoxyComment(TI->DocClass, "");

            if (TI->DocVars)
            {
                Out << FormatDoxyComment("\n"
                    "Note that the variables of this class (also referred to as \"Public Attributes\" or \"Member Data\")\n"
                    "must be used with the get() and set() methods at this time -- see get() and set() for details.", "");
            }

            std::string InfoNew = "";

            if (&TIM == &cf::GuiSys::GetWindowTIM())
            {
                InfoNew = std::string(
                    "\n"
                    "If you would like to create a new window explicitly "
                    "(those defined in the CaWE GUI Editor are instantiated automatically), "
                    "use GuiT::new():\n"
                    "\\code{.lua}\n"
                    "    local win = gui:new(\"") + ClassName + std::string("\", \"my_window\")\n"
                    "\\endcode\n");
            }
            else if (&TIM == &cf::GameSys::GetGameSysEntityTIM())
            {
                InfoNew = std::string(
                    "\n"
                    "If you would like to create a new entity explicitly "
                    "(those defined in the CaWE Map Editor are instantiated automatically), "
                    "use WorldT::new():\n"
                    "\\code{.lua}\n"
                    "    local entity = world:new(\"") + ClassName + std::string("\", \"my_entity\")\n"
                    "\\endcode\n");
            }
            else if (&TIM == &cf::GuiSys::GetComponentTIM())
            {
                InfoNew = std::string(
                    "\n"
                    "If you would like to create a new component of this type explicitly "
                    "(those defined in the CaWE GUI Editor are instantiated automatically), "
                    "use GuiT::new():\n"
                    "\\code{.lua}\n"
                    "    local comp = gui:new(\"") + ClassName + std::string("\")\n"
                    "\\endcode\n");
            }
            else if (&TIM == &cf::GameSys::GetComponentTIM())
            {
                InfoNew = std::string(
                    "\n"
                    "If you would like to create a new component of this type explicitly "
                    "(those defined in the CaWE Map Editor are instantiated automatically), "
                    "use WorldT::new():\n"
                    "\\code{.lua}\n"
                    "    local comp = world:new(\"") + ClassName + std::string("\")\n"
                    "\\endcode\n");
            }

            if (InfoNew != "" && !TI->Child)    // Only do this for "leaf" classes.
                Out << FormatDoxyComment(InfoNew.c_str(), "");

            // We need this so that the "Event Handlers (Callbacks)" group is not made a sub-group of
            // "Public Member Functions", but at the same level (next to it) instead.
            Out << "/// @nosubgrouping\n";

            Out << "/// @cppName{cf," << Namespace << "," << ClassName << "}\n";
            Out << "class " << ClassName;
            if (TI->Base)
            {
                if (std::string(TI->BaseClassName, 0, ColonPos + 2) == Namespace + "::")
                {
                    Out << " : public " << &TI->BaseClassName[ColonPos + 2];
                }
                else
                {
                    Out << " : public " << TI->BaseClassName;
                }
            }
            Out << "\n";
            Out << "{\n";

            WriteDoxyMethods(Out, TI);
            WriteDoxyCallbacks(Out, TI);

            // Write variables.
            if (&TIM == &cf::GuiSys::GetComponentTIM())
            {
                IntrusivePtrT<cf::GuiSys::ComponentBaseT> Comp = static_cast<cf::GuiSys::ComponentBaseT*>(
                    TI->CreateInstance(
                        cf::TypeSys::CreateParamsT()));

                cf::TypeSys::VarManT& VarMan = Comp->GetMemberVars();

                WriteDoxyVars(Out, VarMan, TI);
            }
            else if (&TIM == &cf::GameSys::GetComponentTIM())
            {
                IntrusivePtrT<cf::GameSys::ComponentBaseT> Comp = static_cast<cf::GameSys::ComponentBaseT*>(
                    TI->CreateInstance(
                        cf::TypeSys::CreateParamsT()));

                cf::TypeSys::VarManT& VarMan = Comp->GetMemberVars();

                WriteDoxyVars(Out, VarMan, TI);
            }

            Out << "};\n";
            Out.flush();
        }

        Out << "\n";
        Out << "\n";
        Out << "}   // namespace " << CurrentNamespace << "\n";
    }
}


/// This is an auxiliary method for creating Lua scripting documentation for the registered classes.
/// Assuming that the classes registered with this type info manager provide methods for access from Lua scripts,
/// this method creates Doxygen input files ("fake headers") that documentation writers can complete to create
/// related reference documentation.
void AppCaWE::WriteLuaDoxygenHeaders() const
{
    WriteClassHierarchy("Doxygen/scripting/tmpl/GameWorld.hpp",      cf::GameSys::GetWorldTIM());
    WriteClassHierarchy("Doxygen/scripting/tmpl/GameEntities.hpp",   cf::GameSys::GetGameSysEntityTIM());
    WriteClassHierarchy("Doxygen/scripting/tmpl/GameComponents.hpp", cf::GameSys::GetComponentTIM());

    WriteClassHierarchy("Doxygen/scripting/tmpl/GuiGui.hpp",        cf::GuiSys::GetGuiTIM());
    WriteClassHierarchy("Doxygen/scripting/tmpl/GuiWindows.hpp",    cf::GuiSys::GetWindowTIM());
    WriteClassHierarchy("Doxygen/scripting/tmpl/GuiComponents.hpp", cf::GuiSys::GetComponentTIM());
}
