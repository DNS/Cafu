/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "Options.hpp"
#include "GameConfig.hpp"

#include "wx/wx.h"
#include "wx/confbase.h"
#include "wx/datetime.h"
#include "wx/fileconf.h"
#include "wx/stdpaths.h"
#include "wx/dir.h"
#include "wx/filename.h"


OptionsT::~OptionsT()
{
    DeleteGameConfigs();
}


static bool CompareGameConfigs(GameConfigT* const& GC1, GameConfigT* const& GC2)
{
    return wxStricmp(GC1->Name, GC2->Name)<0;
}


static wxString FindTool(const wxString& Tool, const wxString& Fallback)
{
#ifdef __WXMSW__
    const char* Compilers[] = { "vc14", "vc12", "vc11", "vc10", "vc9", "vc8", NULL };
    const char* Archs[]     = { "x64", "x86", NULL };

    for (const char** Compiler = Compilers; *Compiler; Compiler++)
        for (const char** Arch = Archs; *Arch; Arch++)
        {
            const wxString Path = wxString("build/win32/") + (*Compiler) + "/" + (*Arch) + "/release/" + Tool;

            if (wxFileExists(Path)) return Path;
        }
#else
    const char* Compilers[] = { "g++", NULL };

    for (const char** Compiler = Compilers; *Compiler; Compiler++)
    {
        const wxString Path = wxString("build/linux2/") + (*Compiler) + "/release/" + Tool;

        if (wxFileExists(Path)) return Path;
    }
#endif

    return Fallback;
}


void OptionsT::Init()
{
    time_t Ticks=wxConfigBase::Get()->Read("Configured/Installed", 0L);

    if (Ticks==0)
    {
        // If we are reading this config for the very first time,
        // set the "ticks since installed" to the current time.
        Ticks=wxDateTime::Now().GetTicks();
        wxConfigBase::Get()->Write("Configured/Installed", long(Ticks));
    }

    DaysSinceInstalled=wxDateTime::Now().Subtract(wxDateTime(Ticks)).GetDays();


    // Read settings.
    wxConfigBase::Get()->SetPath("General");
    wxConfigBase::Get()->Read("Undo Levels",       &general.UndoLevels,        50   );
    wxConfigBase::Get()->Read("Locking Textures",  &general.LockingTextures,   true );
    wxConfigBase::Get()->Read("UVs Face Aligned",  &general.NewUVsFaceAligned, false);
#ifdef __WXMSW__
    wxConfigBase::Get()->Read("Engine Executable", &general.EngineExe, FindTool("Ca3DE/Cafu.exe",      "Cafu.exe"));
    wxConfigBase::Get()->Read("BSP Executable",    &general.BSPExe,    FindTool("CaBSP/CaBSP.exe",     "CaBSP.exe"));
    wxConfigBase::Get()->Read("Light Executable",  &general.LightExe,  FindTool("CaLight/CaLight.exe", "CaLight.exe"));
    wxConfigBase::Get()->Read("PVS Executable",    &general.PVSExe,    FindTool("CaPVS/CaPVS.exe",     "CaPVS.exe"));
#else
    wxConfigBase::Get()->Read("Engine Executable", &general.EngineExe, FindTool("Ca3DE/Cafu",      "./Cafu"));
    wxConfigBase::Get()->Read("BSP Executable",    &general.BSPExe,    FindTool("CaBSP/CaBSP",     "./CaBSP"));
    wxConfigBase::Get()->Read("Light Executable",  &general.LightExe,  FindTool("CaLight/CaLight", "./CaLight"));
    wxConfigBase::Get()->Read("PVS Executable",    &general.PVSExe,    FindTool("CaPVS/CaPVS",     "./CaPVS"));
#endif
    wxConfigBase::Get()->SetPath("..");

    wxConfigBase::Get()->SetPath("Views2D");
    wxConfigBase::Get()->Read("Draw Vertices",     &view2d.DrawVertices,      true );
    wxConfigBase::Get()->Read("SelectByHandles",   &view2d.SelectByHandles,   false);
    wxConfigBase::Get()->Read("ShowEntityInfo",    &view2d.ShowEntityInfo,    true );
    wxConfigBase::Get()->Read("ShowEntityTargets", &view2d.ShowEntityTargets, false);
    wxConfigBase::Get()->Read("UseGroupColors",    &view2d.UseGroupColors,    true );
    wxConfigBase::Get()->SetPath("..");

    wxConfigBase::Get()->SetPath("Views3D");
    wxConfigBase::Get()->Read("Reverse Y",         &view3d.ReverseY,          false);
    wxConfigBase::Get()->Read("BackPlane",         &view3d.BackPlane,         6000);
    wxConfigBase::Get()->Read("ModelDistance",     &view3d.ModelDistance,     3000);
    wxConfigBase::Get()->Read("AnimateModels",     &view3d.AnimateModels,     true);
    wxConfigBase::Get()->Read("MaxCameraVelocity", &view3d.MaxCameraVelocity, 1000);
    wxConfigBase::Get()->Read("TimeToMaxSpeed",    &view3d.TimeToMaxSpeed,    500);
    wxConfigBase::Get()->Read("MouseSensitivity",  &view3d.MouseSensitivity,  0.4f);
    wxConfigBase::Get()->Read("SplitPlanesDepth",  &view3d.SplitPlanesDepth,  0);
    wxConfigBase::Get()->SetPath("..");

    wxConfigBase::Get()->SetPath("Grid");
    wxConfigBase::Get()->Read("InitialSpacing",    &Grid.InitialSpacing,    8);
    wxConfigBase::Get()->Read("MinPixelSpacing",   &Grid.MinPixelSpacing,   4);
    wxConfigBase::Get()->Read("UseDottedGrid",     &Grid.UseDottedGrid,     false);
    wxConfigBase::Get()->Read("ShowHighlight1",    &Grid.ShowHighlight1,    true);
    wxConfigBase::Get()->Read("SpacingHighlight1", &Grid.SpacingHighlight1, 64);
    wxConfigBase::Get()->Read("ShowHighlight2",    &Grid.ShowHighlight2,    true);
    wxConfigBase::Get()->Read("SpacingHighlight2", &Grid.SpacingHighlight2, 1024);
    wxConfigBase::Get()->Read("ColorBackground",   &Grid.ColorBackground,   wxColour(0, 0, 0));
    wxConfigBase::Get()->Read("ColorBaseGrid",     &Grid.ColorBaseGrid,     wxColour(32, 32, 32));
    wxConfigBase::Get()->Read("ColorHighlight1",   &Grid.ColorHighlight1,   wxColour(64, 64, 64));
    wxConfigBase::Get()->Read("ColorHighlight2",   &Grid.ColorHighlight2,   wxColour(90, 53, 37));
    wxConfigBase::Get()->Read("ColorAxes",         &Grid.ColorAxes,         wxColour(37, 90, 37));
    wxConfigBase::Get()->SetPath("..");

    // Assign the remaining colors.
    colors.Vertex       =wxColour(255, 255, 255);
    colors.ToolHandle   =wxColour(255, 255, 255);
    colors.ToolDrag     =wxColour(255, 255, 0);
    colors.ToolSelection=wxColour(255, 0, 0);
    colors.Selection    =wxColour(255, 0, 0);
    colors.SelectedFace =wxColour(220, 0, 0);
    colors.SelectedEdge =wxColour(250, 220, 0);
    colors.ToolMorph    =wxColour(255, 0, 0);


    wxFileConfig CfgFile(wxTheApp->GetAppName(), wxTheApp->GetVendorName(), wxStandardPaths::Get().GetUserDataDir()+"/Games.config");
    wxDir        GamesDir("Games");     // Should also work under Linux, otherwise use "./Games".
    wxString     GameName;

    // Iterate through all subdirectories of "Games/" and create a game configuration for each.
    // The creation of a game configuration succeeds (that is, no exception is thrown) only when
    // the related "EntityClassDefs.lua" script could properly be loaded and processed.
    // Other game configuration specific data is initialized from the relevant section from the CfgFile.
    for (bool more=GamesDir.GetFirst(&GameName, "", wxDIR_DIRS); more; more=GamesDir.GetNext(&GameName))
    {
        CfgFile.SetPath("GameConfig_"+GameName);

        try
        {
            GameConfigs.PushBack(new GameConfigT(CfgFile, GameName, "Games/"+GameName));
        }
        catch (const GameConfigT::InitErrorT&)
        {
            // Just do nothing but warn the user if there was an error initializing the game config,
            // that is, the game config will not be pushed back to the GameConfigs array.
            wxMessageBox("Configuration for game \""+GameName+"\" could not be loaded!", "Warning", wxOK | wxICON_EXCLAMATION);
        }

        CfgFile.SetPath("..");
    }

    GameConfigs.QuickSort(CompareGameConfigs);

    // If we couldn't load any game configurations, pop up the options screen.
    /* if (GameConfigs.Size()==0)
    {
        wxMessageBox("I scanned the Games subdirectory for game configurations,\n"
                     "but didn't find any.\n"
                     "You need to have at least one game configuration inside your\n"
                     "Games directory for the editor to work.\n"
                     "Please re-install Cafu or contact the Cafu forums for help.", "No game configurations found", wxOK | wxICON_ERROR);
    } */
}


void OptionsT::Write() const
{
    wxConfigBase::Get()->SetPath("General");
    wxConfigBase::Get()->Write("Undo Levels",       general.UndoLevels       );
    wxConfigBase::Get()->Write("Locking Textures",  general.LockingTextures  );
    wxConfigBase::Get()->Write("UVs Face Aligned",  general.NewUVsFaceAligned);
    wxConfigBase::Get()->Write("Engine Executable", general.EngineExe        );
    wxConfigBase::Get()->Write("BSP Executable",    general.BSPExe           );
    wxConfigBase::Get()->Write("Light Executable",  general.LightExe         );
    wxConfigBase::Get()->Write("PVS Executable",    general.PVSExe           );
    wxConfigBase::Get()->SetPath("..");

    wxConfigBase::Get()->SetPath("Views2D");
    wxConfigBase::Get()->Write("Draw Vertices",     view2d.DrawVertices     );
    wxConfigBase::Get()->Write("SelectByHandles",   view2d.SelectByHandles  );
    wxConfigBase::Get()->Write("ShowEntityInfo",    view2d.ShowEntityInfo   );
    wxConfigBase::Get()->Write("ShowEntityTargets", view2d.ShowEntityTargets);
    wxConfigBase::Get()->Write("UseGroupColors",    view2d.UseGroupColors   );
    wxConfigBase::Get()->SetPath("..");

    wxConfigBase::Get()->SetPath("Views3D");
    wxConfigBase::Get()->Write("Reverse Y",         view3d.ReverseY         );
    wxConfigBase::Get()->Write("BackPlane",         view3d.BackPlane        );
    wxConfigBase::Get()->Write("ModelDistance",     view3d.ModelDistance    );
    wxConfigBase::Get()->Write("AnimateModels",     view3d.AnimateModels    );
    wxConfigBase::Get()->Write("MaxCameraVelocity", view3d.MaxCameraVelocity);
    wxConfigBase::Get()->Write("TimeToMaxSpeed",    view3d.TimeToMaxSpeed   );
    wxConfigBase::Get()->Write("MouseSensitivity",  view3d.MouseSensitivity );
    wxConfigBase::Get()->Write("SplitPlanesDepth",  view3d.SplitPlanesDepth );
    wxConfigBase::Get()->SetPath("..");

    wxConfigBase::Get()->SetPath("Grid");
    wxConfigBase::Get()->Write("InitialSpacing",    Grid.InitialSpacing     );
    wxConfigBase::Get()->Write("MinPixelSpacing",   Grid.MinPixelSpacing    );
    wxConfigBase::Get()->Write("UseDottedGrid",     Grid.UseDottedGrid      );
    wxConfigBase::Get()->Write("ShowHighlight1",    Grid.ShowHighlight1     );
    wxConfigBase::Get()->Write("SpacingHighlight1", Grid.SpacingHighlight1  );
    wxConfigBase::Get()->Write("ShowHighlight2",    Grid.ShowHighlight2     );
    wxConfigBase::Get()->Write("SpacingHighlight2", Grid.SpacingHighlight2  );
    wxConfigBase::Get()->Write("ColorBackground",   Grid.ColorBackground    );
    wxConfigBase::Get()->Write("ColorBaseGrid",     Grid.ColorBaseGrid      );
    wxConfigBase::Get()->Write("ColorHighlight1",   Grid.ColorHighlight1    );
    wxConfigBase::Get()->Write("ColorHighlight2",   Grid.ColorHighlight2    );
    wxConfigBase::Get()->Write("ColorAxes",         Grid.ColorAxes          );
    wxConfigBase::Get()->SetPath("..");

    // Write the game configs. This is in an external file so we can distribute it with CaWE as a set of defaults.
    wxFileConfig CfgFile(wxTheApp->GetAppName(), wxTheApp->GetVendorName(), wxStandardPaths::Get().GetUserDataDir()+"/Games.config");

    for (unsigned long ConfigNr=0; ConfigNr<GameConfigs.Size(); ConfigNr++)
    {
        // Each game configuration is stored in an own group (directory).
        CfgFile.SetPath("GameConfig_"+GameConfigs[ConfigNr]->Name);
        GameConfigs[ConfigNr]->Save(CfgFile);
        CfgFile.SetPath("..");
    }
}


void OptionsT::DeleteGameConfigs()
{
    for (unsigned long i=0; i<GameConfigs.Size(); i++)
        delete GameConfigs[i];

    GameConfigs.Clear();
}
