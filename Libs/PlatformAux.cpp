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


#ifdef SCONS_BUILD_DIR
std::string PlatformAux::GetEnvFileSuffix()
{
    // This is just a lousy way to say that this method is obsolete with the new SCons build system!
    return std::string("_scons");
}
#else
std::string PlatformAux::GetEnvFileSuffix()
{
    std::string Suffix="_";

#if defined(_WIN32)
    // We are on the Win32 platform.
    Suffix+="win32";

    #if defined(__WATCOMC__)
        // Using the OpenWatcom C/C++ compiler.
        Suffix+="_ow";
    #elif defined(_MSC_VER)
        // Using the Microsoft Visual C++ compiler.
        Suffix+="_vc60";
    #else
        Suffix+="_unknown";
    #endif
#elif __linux__ && __i386__
    // We are on the Linux i386 platform.
    Suffix+="li686";

    #if __GNUG__    // This is equivalent to testing (__GNUC__ && __cplusplus).
        // Using the g++ compiler.
        // See http://www.delorie.com/gnu/docs/gcc/cpp_toc.html for documentation about the C preprocessor.
        Suffix+="_g++";
    #else
        Suffix+="_unknown";
    #endif
#else
    Suffix+="unknown_unknown";
#endif

#if defined(DEBUG)
    Suffix+="_d";
#else
    Suffix+="_r";
#endif

    return Suffix;
}
#endif


static void GetDLLs(const std::string& Path, const std::string& Prefix, ArrayT<std::string>& FoundDLLs)
{
    if (Path=="") return;

    #if defined(_WIN32) && defined(_MSC_VER)
    {
        WIN32_FIND_DATA FindFileData;

        HANDLE hFind=FindFirstFile((Path+"\\*").c_str(), &FindFileData);
        if (hFind==INVALID_HANDLE_VALUE) return;

        do
        {
            if (!_stricmp(FindFileData.cFileName, "."  )) continue;
            if (!_stricmp(FindFileData.cFileName, ".." )) continue;
            if (!_stricmp(FindFileData.cFileName, "cvs")) continue;

            // If FindFileData.cFileName doesn't begin with Prefix, continue.
            if (std::string(FindFileData.cFileName, Prefix.length())!=Prefix) continue;

            std::string DLLName=Path+"/"+FindFileData.cFileName;
#ifdef SCONS_BUILD_DIR
            const std::string Suffix=".dll";
#else
            const std::string Suffix=GetEnvFileSuffix()+".dll"; // Console->Print("Suffix "+Suffix+", DLLName "+DLLName+"\n");
#endif

            // If FindFileData.cFileName doesn't end with Suffix, continue.
            if (DLLName.length()<Suffix.length()) continue;
            if (std::string(DLLName.c_str()+DLLName.length()-Suffix.length())!=Suffix) continue;

            FoundDLLs.PushBack(DLLName);
        } while (FindNextFile(hFind, &FindFileData)!=0);

        if (GetLastError()==ERROR_NO_MORE_FILES) FindClose(hFind);
    }
    #else
    {
        DIR* Dir=opendir(Path.c_str());
        if (!Dir) return;

        for (dirent* DirEnt=readdir(Dir); DirEnt!=NULL; DirEnt=readdir(Dir))
        {
            if (!strcasecmp(DirEnt->d_name, "."  )) continue;
            if (!strcasecmp(DirEnt->d_name, ".." )) continue;
            if (!strcasecmp(DirEnt->d_name, "cvs")) continue;

            // If FindFileData.cFileName doesn't begin with LibPrefix, continue.
            const std::string LibPrefix="lib"+Prefix;
            if (std::string(DirEnt->d_name, LibPrefix.length())!=LibPrefix) continue;

            // For portability, only the 'd_name' member of a 'dirent' may be accessed.
            std::string DLLName=Path+"/"+DirEnt->d_name;
#ifdef SCONS_BUILD_DIR
            const std::string Suffix =".so";
#else
            const std::string Suffix =GetEnvFileSuffix()+".dll";
#endif

            // If FindFileData.cFileName doesn't end with Suffix, continue.
            if (DLLName.length()<Suffix.length()) continue;
            if (std::string(DLLName.c_str()+DLLName.length()-Suffix.length())!=Suffix) continue;

            FoundDLLs.PushBack(DLLName);
        }

        closedir(Dir);
    }
    #endif
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
