/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_SOUNDSYS_SOUND_SHADER_MANAGER_INTERFACE_HPP_INCLUDED
#define CAFU_SOUNDSYS_SOUND_SHADER_MANAGER_INTERFACE_HPP_INCLUDED

#include "Templates/Array.hpp"

#include <string>


class SoundShaderT;


/// This is an interface to the sound shader manager.
/// The interface is specified as ABC in order to be able to share the sound shader manager across exe/dll boundaries.
/// (Note that sharing across exe/dll boundaries is probably not needed: The SoundShaderManagerI is just a helper
///  for getting SoundShaderTs from script files. Thus we could also share (pointers to) arrays of SoundShaderTs directly.
///  (MODs should not register their own sound shaders, as the engine registeres anything in the *SoundShader* dir. anyway.)
///  It is just the GetSoundShader() "search" function that makes sharing the SoundShaderManagerI interesting.)
class SoundShaderManagerI
{
    public:

    /// Register a sound shader script file by parsing all sound shaders from the list and adding them to the manager.
    /// @param ScriptFile Path to the file that contains the sound shader definitions.
    /// @param ModDir The directory of the MOD the sound shader script is registered for relative to the executables directory.
    /// @return Array of new created sound shaders or empty array if a problem occured or no new sound shader definitions
    ///         were found in the script.
    virtual ArrayT<const SoundShaderT*> RegisterSoundShaderScript(const std::string& ScriptFile, const std::string& ModDir)=0;

    /// Registers all ".caud" files inside a directory.
    /// @param Directory The path from which sound shader script files should be registered.
    /// @param ModDir The directory of the MOD for which the sound shader script in this directory are registered relative to the executables directory.
    /// @param Recurse Determines if subdirectories are searched for ".caud" files recusively.
    /// @return Array of new created sound shaders or empty array if a problem occured or no new sound shader definitions
    ///         were found in the script.
    virtual ArrayT<const SoundShaderT*> RegisterSoundShaderScriptsInDir(const std::string& Directory, const std::string& ModDir, bool Recurse=true)=0;

    /// Searches for the shader specified by Name and returns it.
    /// @param Name The name of this sound shader as it is defined in the sound shader script.
    ///             If no sound shader by this name is found the manager tries to interpret the Name as a filename to
    ///             an audio file and automatically creates a default shader for it.
    /// @return Pointer to the found sound shader or NULL if no sound shader with this name is registered and no default
    ///         shader could be created.
    virtual const SoundShaderT* GetSoundShader(const std::string& Name)=0;

    /// Virtual destructor, so that nothing can go wrong and even g++ is happy.
    virtual ~SoundShaderManagerI() { }
};


/// A global pointer to an implementation of the SoundShaderManagerI interface.
///
/// Each module (exe or dll) that uses this pointer must somewhere provide exactly one definition for it (none is provided by the SoundSys).
/// That is, typically the main.cpp or similar file of each exe and dll must contain a line like
///     SoundShaderManagerI* SoundShaderManager=NULL;
/// or else the module will not link successfully due to an undefined symbol.
///
/// Exe files will then want to reset this pointer to an instance of a SoundShaderManagerImplT during their initialization
/// e.g. by code like:   SoundShaderManager=new SoundShaderManagerImplT;
/// Note that the SoundShaderManagerImplT ctor may require that other interfaces (e.g. the Console) have been inited first.
///
/// Dlls typically get one of their init functions called immediately after they have been loaded.
/// By doing so, the exe passes a pointer to its above instance to the dll, which in turn copies it to its SoundShaderManager variable.
extern SoundShaderManagerI* SoundShaderManager;

#endif
