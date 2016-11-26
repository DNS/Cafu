/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "PlatformAux.hpp"
#include "Templates/Array.hpp"
#include "ConsoleCommands/Console.hpp"
#include "FileSys/FileMan.hpp"
#include "MaterialSystem/Renderer.hpp"
#include "SoundSystem/SoundSys.hpp"

#include <cassert>

#ifdef _WIN32
#elif __linux__
#include <cstdio>
#include <cstring>
#include <dirent.h>
#include <dlfcn.h>
#define __stdcall
#define GetProcAddress dlsym
#define FreeLibrary dlclose
#endif


std::vector<std::string> PlatformAux::GetDirectory(const std::string& Name, char Filter)
{
    std::vector<std::string> Items;

    if (Name == "")
        return Items;

#ifdef _WIN32
    WIN32_FIND_DATA FindFileData;
    HANDLE          FindHandle = FindFirstFile((Name + "\\*").c_str(), &FindFileData);

    if (FindHandle == INVALID_HANDLE_VALUE)
        return Items;

    do
    {
        if (strcmp(FindFileData.cFileName, "." ) == 0) continue;
        if (strcmp(FindFileData.cFileName, "..") == 0) continue;

        if (Filter)
        {
            const bool IsDir = (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;

            if (Filter == 'f' &&  IsDir) continue;
            if (Filter == 'd' && !IsDir) continue;
        }

        Items.push_back(FindFileData.cFileName);
    }
    while (FindNextFile(FindHandle, &FindFileData));

    if (GetLastError() != ERROR_NO_MORE_FILES)
        Console->Warning("Error in GetDirectory() while enumerating directory entries.\n");

    FindClose(FindHandle);
#else
    DIR* Dir = opendir(Name.c_str());

    if (!Dir)
        return Items;

    for (dirent* DirEnt = readdir(Dir); DirEnt; DirEnt = readdir(Dir))
    {
        if (strcmp(DirEnt->d_name, "." ) == 0) continue;
        if (strcmp(DirEnt->d_name, "..") == 0) continue;

        if (Filter)
        {
            DIR* TempDir = opendir((Name + "/" + DirEnt->d_name).c_str());
            const bool IsDir = (TempDir != NULL);

            if (TempDir)
                closedir(TempDir);

            if (Filter == 'f' &&  IsDir) continue;
            if (Filter == 'd' && !IsDir) continue;
        }

        // For portability, only the 'd_name' member of a 'dirent' may be accessed.
        Items.push_back(DirEnt->d_name);
    }

    closedir(Dir);
#endif

    return Items;
}


static void GetDLLs(const std::string& Path, const std::string& Prefix, ArrayT<std::string>& FoundDLLs)
{
    const std::vector<std::string> Items = PlatformAux::GetDirectory(Path);

#if defined(_WIN32)
    const std::string LibPrefix = Prefix;
    const std::string Suffix=".dll";
#else
    const std::string LibPrefix = "lib" + Prefix;
    const std::string Suffix =".so";
#endif

    for (size_t i = 0; i < Items.size(); i++)
    {
        const std::string& DLLName = Items[i];

        // If DLLName doesn't begin with LibPrefix, continue.
        if (std::string(DLLName, 0, LibPrefix.length()) != LibPrefix) continue;

        // If DLLName doesn't end with Suffix, continue.
        if (DLLName.length() < Suffix.length()) continue;
        if (std::string(DLLName, DLLName.length() - Suffix.length()) != Suffix) continue;

        FoundDLLs.PushBack(Path + "/" + DLLName);
    }
}


MatSys::RendererI* PlatformAux::GetRenderer(const std::string& DLLName, HMODULE& OutRendererDLL)
{
    #ifdef _WIN32
        OutRendererDLL=LoadLibrary(DLLName.c_str());
    #else
        // Note that RTLD_GLOBAL must *not* be passed-in here, or else we get in trouble with subsequently loaded libraries.
        // (E.g. it causes dlsym(OutRendererDLL, "GetRenderer") to return identical results for different OutRendererDLLs.)
        // Please refer to the man page of dlopen for more details.
        OutRendererDLL=dlopen(DLLName.c_str(), RTLD_NOW);
        if (!OutRendererDLL) Console->Print(std::string(dlerror()) + ", ");
    #endif

    if (!OutRendererDLL) { Console->Print("FAILED - could not load the library at "+DLLName+".\n"); return NULL; }


    typedef MatSys::RendererI* (__stdcall *GetRendererT)(cf::ConsoleI* Console_, cf::FileSys::FileManI* FileMan_);

    #if defined(_WIN32) && !defined(_WIN64)
        GetRendererT GetRendererFunc=(GetRendererT)GetProcAddress(OutRendererDLL, "_GetRenderer@8");
    #else
        GetRendererT GetRendererFunc=(GetRendererT)GetProcAddress(OutRendererDLL, "GetRenderer");
    #endif

    if (!GetRendererFunc) { Console->Print("FAILED - could not get the address of the GetRenderer() function.\n"); FreeLibrary(OutRendererDLL); return NULL; }


    // When we get here, the console and the file man must already have been initialized.
    assert(Console!=NULL);
    assert(cf::FileSys::FileMan!=NULL);

    MatSys::RendererI* Renderer=GetRendererFunc(Console, cf::FileSys::FileMan);

    if (!Renderer) { Console->Print("FAILED - could not get the renderer.\n"); FreeLibrary(OutRendererDLL); return NULL; }
    if (!Renderer->IsSupported()) { Console->Print("FAILED - renderer says it's not supported.\n"); FreeLibrary(OutRendererDLL); return NULL; }

    return Renderer;
}


MatSys::RendererI* PlatformAux::GetBestRenderer(HMODULE& OutRendererDLL)
{
#ifdef SCONS_BUILD_DIR
    #define QUOTE(str) QUOTE_HELPER(str)
    #define QUOTE_HELPER(str) #str
    std::string Path=std::string("Libs/")+QUOTE(SCONS_BUILD_DIR)+"/MaterialSystem";
    #undef QUOTE
    #undef QUOTE_HELPER
#else
    std::string Path="Renderers";
#endif
    ArrayT<std::string> DLLNames;

    Console->Print("\n");
    Console->Print("Scanning cwd for all available renderers...\n");
    GetDLLs(".", "Renderer", DLLNames);

    if (DLLNames.Size()==0)
    {
        Console->Print("Scanning "+Path+" for all available renderers...\n");
        GetDLLs(Path, "Renderer", DLLNames);
    }


    unsigned long BestDLLIndex= 0;      // Index into DLLNames.
    int           BestPrefNr  =-1;      // The preference number of the renderer related to BestDLLIndex.

    for (unsigned long DLLNr=0; DLLNr<DLLNames.Size(); DLLNr++)
    {
        Console->Print(DLLNames[DLLNr]+" ... ");

        HMODULE RendererDLL;
        MatSys::RendererI* Renderer=GetRenderer(DLLNames[DLLNr], RendererDLL);

        if (!Renderer) continue;

        int PrefNr=Renderer->GetPreferenceNr();

        Renderer=NULL;
        FreeLibrary(RendererDLL);

        if (PrefNr<10)
        {
            // We don't want the Null renderer to be possibly selected for client rendering
            // (which can happen in the presence of other errors).
            // It would only confuse and worry users to sit in front of a black, apparently frozen screen.
            Console->Print(cf::va("SUCCESS - but excluded from auto-selection (Pref# %i).\n", PrefNr));
            continue;
        }

        if (PrefNr>BestPrefNr)
        {
            Console->Print(cf::va("SUCCESS - %s renderer (Pref# %i).\n", BestPrefNr<0 ? "first supported" : "higher preference", PrefNr));

            BestDLLIndex=DLLNr;
            BestPrefNr  =PrefNr;
        }
        else Console->Print(cf::va("SUCCESS - but no higher preference (Pref# %i).\n", PrefNr));
    }

    if (BestPrefNr==-1)
    {
        Console->Print("No renderer qualified.\n");
        return NULL;
    }

    Console->Print("Reloading previously auto-selected renderer "+DLLNames[BestDLLIndex]+" ...\n");
    return GetRenderer(DLLNames[BestDLLIndex], OutRendererDLL);
}


MatSys::TextureMapManagerI* PlatformAux::GetTextureMapManager(HMODULE RendererDLL)
{
    typedef MatSys::TextureMapManagerI* (__stdcall *GetTMMT)();

    #if defined(_WIN32) && !defined(_WIN64)
        GetTMMT GetTMM=(GetTMMT)GetProcAddress(RendererDLL, "_GetTextureMapManager@0");
    #else
        GetTMMT GetTMM=(GetTMMT)GetProcAddress(RendererDLL, "GetTextureMapManager");
    #endif

    if (!GetTMM) { Console->Print("FAILED - could not get the address of the GetTextureMapManager() function.\n"); return NULL; }

    return GetTMM();
}


SoundSysI* PlatformAux::GetSoundSys(const std::string& DLLName, HMODULE& OutSoundSysDLL)
{
    #ifdef _WIN32
        OutSoundSysDLL=LoadLibrary(DLLName.c_str());
    #else
        // Note that RTLD_GLOBAL must *not* be passed-in here, or else we get in trouble with subsequently loaded libraries.
        // (E.g. it causes dlsym(OutSoundSysDLL, "GetSoundSys") to return identical results for different OutSoundSysDLLs.)
        // Please refer to the man page of dlopen for more details.
        OutSoundSysDLL=dlopen(DLLName.c_str(), RTLD_NOW);
        if (!OutSoundSysDLL) Console->Print(std::string(dlerror()) + ", ");
    #endif

    if (!OutSoundSysDLL) { Console->Print("FAILED - could not load the library at "+DLLName+".\n"); return NULL; }


    typedef SoundSysI* (__stdcall *GetSoundSys)(cf::ConsoleI* Console_, cf::FileSys::FileManI* FileMan_);

    #if defined(_WIN32) && !defined(_WIN64)
        GetSoundSys GetSoundSysFunc=(GetSoundSys)GetProcAddress(OutSoundSysDLL, "_GetSoundSys@8");
    #else
        GetSoundSys GetSoundSysFunc=(GetSoundSys)GetProcAddress(OutSoundSysDLL, "GetSoundSys");
    #endif

    if (!GetSoundSysFunc) { Console->Print("FAILED - could not get the address of the GetSoundSys() function.\n"); FreeLibrary(OutSoundSysDLL); return NULL; }


    // When we get here, the console and the file man must already have been initialized.
    assert(Console!=NULL);
    assert(cf::FileSys::FileMan!=NULL);

    SoundSysI* SoundSys=GetSoundSysFunc(Console, cf::FileSys::FileMan);

    if (!SoundSys) { Console->Print("FAILED - could not get the SoundSys.\n"); FreeLibrary(OutSoundSysDLL); return NULL; }
    if (!SoundSys->IsSupported()) { Console->Print("FAILED - SoundSys says it's not supported.\n"); FreeLibrary(OutSoundSysDLL); return NULL; }

    return SoundSys;
}


SoundSysI* PlatformAux::GetBestSoundSys(HMODULE& OutSoundSysDLL)
{
#ifdef SCONS_BUILD_DIR
    #define QUOTE(str) QUOTE_HELPER(str)
    #define QUOTE_HELPER(str) #str
    std::string Path=std::string("Libs/")+QUOTE(SCONS_BUILD_DIR)+"/SoundSystem";
    #undef QUOTE
    #undef QUOTE_HELPER
#else
    std::string Path="SoundSystem";
#endif
    ArrayT<std::string> DLLNames;

    Console->Print("\n");
    Console->Print("Scanning cwd for all available sound systems...\n");
    GetDLLs(".", "SoundSys", DLLNames);

    if (DLLNames.Size()==0)
    {
        Console->Print("Scanning "+Path+" for all available sound systems...\n");
        GetDLLs(Path, "SoundSys", DLLNames);
    }


    unsigned long BestDLLIndex= 0;      // Index into DLLNames.
    int           BestPrefNr  =-1;      // The preference number of the sound system related to BestDLLIndex.

    for (unsigned long DLLNr=0; DLLNr<DLLNames.Size(); DLLNr++)
    {
        Console->Print(DLLNames[DLLNr]+" ... ");

        HMODULE SoundSysDLL;
        SoundSysI* SoundSys=GetSoundSys(DLLNames[DLLNr], SoundSysDLL);

        if (!SoundSys) continue;

        int PrefNr=SoundSys->GetPreferenceNr();

        SoundSys=NULL;
        FreeLibrary(SoundSysDLL);

        if (PrefNr<10)
        {
            // We don't want the Null sound system to be possibly selected for client
            // (which can happen in the presence of other errors).
            // It would only confuse and worry users to sit in front of a black, apparently frozen screen.
            Console->Print(cf::va("SUCCESS - but excluded from auto-selection (Pref# %i).\n", PrefNr));
            continue;
        }

        if (PrefNr>BestPrefNr)
        {
            Console->Print(cf::va("SUCCESS - %s sound system (Pref# %i).\n", BestPrefNr<0 ? "first supported" : "higher preference", PrefNr));

            BestDLLIndex=DLLNr;
            BestPrefNr  =PrefNr;
        }
        else Console->Print(cf::va("SUCCESS - but no higher preference (Pref# %i).\n", PrefNr));
    }

    if (BestPrefNr==-1)
    {
        Console->Print("No sound system qualified.\n");
        return NULL;
    }

    Console->Print("Reloading previously auto-selected sound system "+DLLNames[BestDLLIndex]+" ...\n");
    return GetSoundSys(DLLNames[BestDLLIndex], OutSoundSysDLL);
}
