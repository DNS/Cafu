/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

/**********************************************/
/***                                        ***/
/*** Cafu Binary Space Partitioning Utility ***/
/***                                        ***/
/***     I think that I shall never see     ***/
/***         a poem lovely as a tree        ***/
/***                                        ***/
/**********************************************/

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <stdarg.h>
#include <unistd.h>
#include <string.h>
#define _stricmp strcasecmp
#endif

#include <time.h>
#include <ctype.h>
#include <stdio.h>

#include "ConsoleCommands/Console.hpp"
#include "ConsoleCommands/ConsoleInterpreter.hpp"
#include "ConsoleCommands/ConsoleStdout.hpp"
#include "FileSys/FileManImpl.hpp"
#include "GameSys/AllComponents.hpp"
#include "GuiSys/GuiResources.hpp"
#include "Templates/Array.hpp"
#include "MaterialSystem/Material.hpp"
#include "MaterialSystem/MaterialManager.hpp"
#include "MaterialSystem/MaterialManagerImpl.hpp"
#include "ClipSys/CollisionModelMan_impl.hpp"
#include "SoundSystem/SoundShaderManagerImpl.hpp"
#include "SoundSystem/SoundSys.hpp"

#include "BspTreeBuilder/BspTreeBuilder.hpp"

#if defined(_WIN32)
    #if defined(_MSC_VER)
        #define vsnprintf _vsnprintf
    #endif
#endif


static cf::ConsoleStdoutT s_ConsoleStdout(true);      // Enable auto-flushing the stdout stream for CaBSP.
cf::ConsoleI* Console = &s_ConsoleStdout;

static cf::FileSys::FileManImplT s_FileManImpl;
cf::FileSys::FileManI* cf::FileSys::FileMan = &s_FileManImpl;

static cf::ClipSys::CollModelManImplT s_CCM;
cf::ClipSys::CollModelManI* cf::ClipSys::CollModelMan = &s_CCM;

ConsoleInterpreterI* ConsoleInterpreter = NULL;
MaterialManagerI*    MaterialManager    = NULL;

static SoundShaderManagerImplT s_SSM;
SoundShaderManagerI* SoundShaderManager = &s_SSM;

SoundSysI* SoundSystem = NULL;


const time_t ProgramStartTime = time(NULL);

// Returns a string with the elapsed time since program start.
// The string is in the format "hh:mm:ss".
const char* GetTimeSinceProgramStart()
{
    const unsigned long TotalSec=(unsigned long)difftime(time(NULL), ProgramStartTime);
    const unsigned long Sec     =TotalSec % 60;
    const unsigned long Min     =(TotalSec/60) % 60;
    const unsigned long Hour    =TotalSec/3600;

    static char TimeString[24];
    sprintf(TimeString, "%2lu:%2lu:%2lu", Hour, Min, Sec);

    return TimeString;
}


static void Error(const char* ErrorText, ...)
{
    va_list ArgList;
    char    ErrorString[256];

    if (ErrorText!=NULL)
    {
        va_start(ArgList, ErrorText);
            vsnprintf(ErrorString, 256, ErrorText, ArgList);
        va_end(ArgList);

        Console->Print(cf::va("\nFATAL ERROR: %s\n", ErrorString));
    }

    Console->Print("Program aborted.\n\n");
    exit(1);
}


static void WriteLogFileEntry(const char* WorldPathName)
{
    char          DateTime [256]="unknown";
    char          HostName [256]="unknown";
    char          WorldName[256]="unknown";
    time_t        Time          =time(NULL);
    unsigned long RunningSec    =(unsigned long)difftime(Time, ProgramStartTime);
    FILE*         LogFile       =fopen("CaBSP.log", "a");

    if (!LogFile) return;

    strftime(DateTime, 256, "%d.%m.%Y %H:%M", localtime(&Time));
    DateTime[255]=0;

#ifdef _WIN32
    unsigned long Dummy=256;
    if (!GetComputerName(HostName, &Dummy)) sprintf(HostName, "unknown (look-up failed).");
#else
    // This function also works on Windows, but sadly requires calls to 'WSAStartup()' and 'WSACleanup()'.
    if (gethostname(HostName, 256)) sprintf(HostName, "unknown (look-up failed).");
#endif
    HostName[255]=0;

    if (WorldPathName)
    {
        // Dateinamen abtrennen (mit Extension).
        size_t i=strlen(WorldPathName);

        while (i>0 && WorldPathName[i-1]!='/' && WorldPathName[i-1]!='\\') i--;
        strncpy(WorldName, WorldPathName+i, 256);
        WorldName[255]=0;

        // Extension abtrennen.
        i=strlen(WorldName);

        while (i>0 && WorldName[i-1]!='.') i--;
        if (i>0) WorldName[i-1]=0;
    }

    // Date, Time, WorldName, TimeForCompletion on [HostName]
    fprintf(LogFile, "%-16s %-16s%3lu:%02lu:%02lu [%-16s]\n", DateTime, WorldName, RunningSec/3600, (RunningSec/60) % 60, RunningSec % 60, HostName);
    fclose(LogFile);
}


void Usage()
{
    Console->Print("USAGE: CaBSP InFile.cmap OutFile.cw [OPTIONS]\n");
    Console->Print("\n");
    Console->Print("Please note that all file names must include their paths and suffixes!\n");

    // The most simple tree means that there is no leak detection and no attempt to fill the world.
    exit(1);
}


#include "../Common/World.hpp"
#include "LoadWorld.cpp"


int main(int ArgC, const char* ArgV[])
{
    cf::GameSys::GetComponentTIM().Init();      // The one-time init of the GameSys components type info manager.
    cf::GameSys::GetGameSysEntityTIM().Init();  // The one-time init of the GameSys entity type info manager.
    cf::GameSys::GetWorldTIM().Init();          // The one-time init of the GameSys world type info manager.

    bool Option_MostSimpleTree = false;
    bool Option_BspSplitFaces  = false;     // Don't split faces when creating a BSP tree.
    bool Option_ChopUpFaces    = true;      // Chop up interpenetrating faces.

    Console->Print(cf::va("\n*** Cafu Binary Space Partitioning Utility, Version 12 (%s) ***\n\n", __DATE__));


    // Initialize the FileMan by mounting the default file system.
    // Note that specifying "./" (instead of "") as the file system description effectively prevents the use of
    // absolute paths like "D:\abc\someDir\someFile.xy" or "/usr/bin/xy". This however should be fine for this application.
    cf::FileSys::FileMan->MountFileSystem(cf::FileSys::FS_TYPE_LOCAL_PATH, "./", "");
    // cf::FileSys::FileMan->MountFileSystem(cf::FileSys::FS_TYPE_ZIP_ARCHIVE, "Games/DeathMatch/Textures/TechDemo.zip", "Games/DeathMatch/Textures/TechDemo/", "Ca3DE");
    // cf::FileSys::FileMan->MountFileSystem(cf::FileSys::FS_TYPE_ZIP_ARCHIVE, "Games/DeathMatch/Textures/SkyDomes.zip", "Games/DeathMatch/Textures/SkyDomes/", "Ca3DE");

    // Parse the command-line.
    // Currently, no extension-checking or other comfort for file names is provided.
    if (ArgC<3) Usage();

    for (int ArgNr=3; ArgNr<ArgC; ArgNr++)
    {
        if (!_stricmp(ArgV[ArgNr], "--most-simple-tree") || !_stricmp(ArgV[ArgNr], "-mst"))
        {
            Option_MostSimpleTree = true;
        }
        else if (!_stricmp(ArgV[ArgNr], "--bsp-split-faces") || !_stricmp(ArgV[ArgNr], "-bsf"))
        {
            Option_BspSplitFaces = true;
        }
        else if (!_stricmp(ArgV[ArgNr], "--dont-chop-up-faces") || !_stricmp(ArgV[ArgNr], "-dcuf"))
        {
            Option_ChopUpFaces = false;
        }
        else if (ArgV[ArgNr][0] == 0)
        {
            // The argument is "", the empty string.
            // This can happen under Linux, when CaBSP is called via wxExecute() with white-space trailing the command string.
        }
        else
        {
            Console->Print(cf::va("Unknown option '%s'.\n", ArgV[ArgNr]));
            Usage();
        }
    }


    std::string GameDirectory=ArgV[2];

    // Determine the game directory, cleverly assuming that the destination file is in "Worlds".
    {
        // Strip the file name and extention off.
        size_t i=GameDirectory.find_last_of("/\\");

        GameDirectory=GameDirectory.substr(0, i==std::string::npos ? 0 : i)+"/..";
    }


    // Setup the global MaterialManager pointer.
    static MaterialManagerImplT MatManImpl;

    MaterialManager=&MatManImpl;

    if (MaterialManager->RegisterMaterialScriptsInDir(GameDirectory+"/Materials", GameDirectory+"/").Size()==0)
    {
        Console->Print(std::string("\nNo materials found in scripts in \"")+GameDirectory+"/Materials\".\n");
        Error("No materials found.");
    }


    ModelManagerT             ModelMan;
    cf::GuiSys::GuiResourcesT GuiRes(ModelMan);
    WorldT                    World;
    ArrayT<Vector3dT>         FloodFillSources;
    ArrayT<Vector3dT>         DrawWorldOutsidePointSamples;
    unsigned int              NumPlayerPrototypes = 0;

    LoadWorld(ArgV[1], GameDirectory, ModelMan, GuiRes, World, FloodFillSources, DrawWorldOutsidePointSamples, NumPlayerPrototypes);

    if (NumPlayerPrototypes == 0)
    {
        Console->Print("\nThere is no entity in this map that can act as a player prototype.\n");
        Console->Print("Technically, a player prototype is an entity with a HumanPlayer component.\n");
        Console->Print("The server refers to this prototype whenever a new player joins a game.\n");
        Console->Print("\nYou can fix this problem by inserting a PlayerPrototype prefab into the map\n");
        Console->Print("(in the Map Editor, use menu \"Prefabs > PlayerPrototype\").\n");

        Error("There is no player prototype entity in this map.");
    }

    if (FloodFillSources.Size() == 0)
    {
        Console->Print("\nThere are no entities in this map that have a PlayerStart component.\n");
        Console->Print("\nYou can fix this problem by inserting one or more PlayerStart prefabs\n");
        Console->Print("into the map (in the Map Editor, use menu \"Prefabs > PlayerStart\").\n");
        Console->Print("Alternatively, add a PlayerStart component to a suitable entity,\n");
        Console->Print("e.g. the one that also acts as the player prototype.\n");

        Error("There are no entities with a PlayerStart component in this map.");
    }

    // What we need:
    // For each entity: The BspTree itself, OutsidePointSamples, FloodFillSources.
    // One common instance, shared for all: LeakDetectMat
    BspTreeBuilderT BspTreeBuilder(World.m_StaticEntityData[0]->m_BspTree, Option_MostSimpleTree, Option_BspSplitFaces, Option_ChopUpFaces);

    BspTreeBuilder.Build(true /*yes, this is the worldspawn entity*/, FloodFillSources, DrawWorldOutsidePointSamples, ArgV[1]);

    try
    {
        Console->Print(cf::va("\n%-50s %s\n", "*** Save World ***", GetTimeSinceProgramStart()));
        Console->Print(std::string(ArgV[2])+"\n");
        World.SaveToDisk(ArgV[2]);

        WriteLogFileEntry(ArgV[2]);
        Console->Print(cf::va("\n%-50s %s\n", "COMPLETED.", GetTimeSinceProgramStart()));
    }
    catch (const WorldT::SaveErrorT& E) { Error(E.Msg); }

    return 0;
}
