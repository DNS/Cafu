/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "SoundShaderManagerImpl.hpp"
#include "SoundShader.hpp"

#include "TextParser/TextParser.hpp"
#include "FileSys/FileMan.hpp"
#include "FileSys/File.hpp"

#ifdef _WIN32
    #include <direct.h>
    #if defined(_MSC_VER)
        #define WIN32_LEAN_AND_MEAN
        #include <windows.h>
    #endif
#else
    #include <cstring>
    #include <dirent.h>
#endif

#include <iostream>


SoundShaderManagerImplT::SoundShaderManagerImplT()
{
}


SoundShaderManagerImplT::~SoundShaderManagerImplT()
{
    for (std::map<std::string, SoundShaderT*>::const_iterator SoundShader=m_SoundShaders.begin(); SoundShader!=m_SoundShaders.end(); SoundShader++)
        delete SoundShader->second;
    m_SoundShaders.clear();
}


ArrayT<const SoundShaderT*> SoundShaderManagerImplT::RegisterSoundShaderScript(const std::string& ScriptFile, const std::string& ModDir)
{
    ArrayT<const SoundShaderT*> NewSoundShaders;

    // Check if script has already been loaded and return empty sound shader array if this is the case.
    for (unsigned long Nr=0; Nr<m_SoundShaderScriptFiles.Size(); Nr++)
        if (m_SoundShaderScriptFiles[Nr]==ScriptFile) return NewSoundShaders;

    m_SoundShaderScriptFiles.PushBack(ScriptFile);

    // Get sound shaders from the script.
    TextParserT TextParser(ScriptFile.c_str(), "({[]}),");

    try
    {
        while (!TextParser.IsAtEOF())
        {
            const std::string Token=TextParser.GetNextToken();

            // If the sound shader cannot be parsed (e.g. due to a syntax error or unknown token),
            // the parsing of the entire file is aborted - the file might be something else than a sound shader script.
            // Even if it was, we cannot easily continue anyway.

            SoundShaderT*  NewSoundShader=new SoundShaderT(Token, TextParser, ModDir);
            SoundShaderT*& TestShader    =m_SoundShaders[NewSoundShader->Name];

            // Check if sound shader with this name already exists.
            if (TestShader==NULL)
            {
                TestShader=NewSoundShader;
                NewSoundShaders.PushBack(NewSoundShader);
            }
            else
            {
                std::cout << "File '"<< ScriptFile << "' sound shader '" << NewSoundShader->Name << "' duplicate definition (ignored).\n";
                delete NewSoundShader;
            }
        }
    }
    catch (const TextParserT::ParseError&)
    {
        std::cout << "Error parsing '" << ScriptFile << "' at input byte " << TextParser.GetReadPosByte() << "\n";
    }

    return NewSoundShaders;
}


ArrayT<const SoundShaderT*> SoundShaderManagerImplT::RegisterSoundShaderScriptsInDir(const std::string& Directory, const std::string& ModDir, bool Recurse)
{
    ArrayT<const SoundShaderT*> NewSoundShaders;

    if (Directory=="") return NewSoundShaders;

#ifdef _MSC_VER
    WIN32_FIND_DATA FindFileData;

    HANDLE hFind=FindFirstFile((Directory+"\\*").c_str(), &FindFileData);

    // MessageBox(NULL, hFind==INVALID_HANDLE_VALUE ? "INV_VALUE!!" : FindFileData.cFileName, "Starting parsing.", MB_ICONINFORMATION);
    if (hFind==INVALID_HANDLE_VALUE) return NewSoundShaders;

    ArrayT<std::string> DirEntNames;

    do
    {
        // MessageBox(NULL, FindFileData.cFileName, "Material Script Found", MB_ICONINFORMATION);
        if (!_stricmp(FindFileData.cFileName, "."  )) continue;
        if (!_stricmp(FindFileData.cFileName, ".." )) continue;
        if (!_stricmp(FindFileData.cFileName, "cvs")) continue;

        DirEntNames.PushBack(Directory+"/"+FindFileData.cFileName);
    } while (FindNextFile(hFind, &FindFileData)!=0);

    if (GetLastError()==ERROR_NO_MORE_FILES) FindClose(hFind);
#else
    DIR* Dir=opendir(Directory.c_str());

    if (!Dir) return NewSoundShaders;

    ArrayT<std::string> DirEntNames;

    for (dirent* DirEnt=readdir(Dir); DirEnt!=NULL; DirEnt=readdir(Dir))
    {
        if (!strcasecmp(DirEnt->d_name, "."  )) continue;
        if (!strcasecmp(DirEnt->d_name, ".." )) continue;
        if (!strcasecmp(DirEnt->d_name, "cvs")) continue;

        // For portability, only the 'd_name' member of a 'dirent' may be accessed.
        DirEntNames.PushBack(Directory+"/"+DirEnt->d_name);
    }

    closedir(Dir);
#endif


    for (unsigned long DENr=0; DENr<DirEntNames.Size(); DENr++)
    {
#ifdef _WIN32
        bool IsDirectory=true;

        FILE* TempFile=fopen(DirEntNames[DENr].c_str(), "r");
        if (TempFile!=NULL)
        {
            // This was probably a file (instead of a directory).
            fclose(TempFile);
            IsDirectory=false;
        }
#else
        bool IsDirectory=false;

        // Sigh. And this doesn't work under Win32... (opendir does not return NULL for files!?!)
        DIR* TempDir=opendir(DirEntNames[DENr].c_str());
        if (TempDir!=NULL)
        {
            // This was a directory.
            closedir(TempDir);
            IsDirectory=true;
        }
#endif

        if (IsDirectory)
        {
            if (Recurse)
                NewSoundShaders.PushBack(RegisterSoundShaderScriptsInDir(DirEntNames[DENr], ModDir));
        }
        else
        {
            if (DirEntNames[DENr].length()>5)
                if (std::string(DirEntNames[DENr].c_str()+DirEntNames[DENr].length()-5)==".caud")
                    NewSoundShaders.PushBack(RegisterSoundShaderScript(DirEntNames[DENr], ModDir));
        }
    }

    return NewSoundShaders;
}


static bool IsFile(const std::string& Name)
{
    cf::FileSys::InFileI* AudioFile=cf::FileSys::FileMan->OpenRead(Name);

    if (AudioFile==NULL)
        return false;

    cf::FileSys::FileMan->Close(AudioFile);
    return true;
}


static bool IsCapture(const std::string& Name)
{
    return Name.find("capture")==0 && Name.length()>8;
}


const SoundShaderT* SoundShaderManagerImplT::GetSoundShader(const std::string& Name)
{
    // Ignore empty names and just return NULL.
    if (Name.empty()) return NULL;

    // Note that I'm *not* just writing   return SoundShaders[Name]   here, because that
    // would implicitly create a NULL entry for every Name that does not actually exist.
    std::map<std::string, SoundShaderT*>::const_iterator It=m_SoundShaders.find(Name);

    if (It!=m_SoundShaders.end()) return It->second;

    // Sound shader not found, try to interpret the name as a filename or a "capture n" string.
    if (IsFile(Name) || IsCapture(Name))
    {
        SoundShaderT* AutoShader=new SoundShaderT(Name);
        AutoShader->AudioFile=Name;

        // Add auto created shader to list of shaders.
        m_SoundShaders[Name]=AutoShader;

        return AutoShader;
    }

    std::cout << "Error auto creating sound shader: File '" << Name << "' not doesn't exist.\n";
    return NULL;
}
