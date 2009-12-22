/*
=================================================================================
This file is part of Cafu, the open-source game and graphics engine for
multiplayer, cross-platform, real-time 3D action.
$Id$

Copyright (C) 2002-2010 Carsten Fuchs Software.

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

#ifndef _SOUNDSYS_SOUND_SHADER_MANAGER_INTERFACE_HPP_
#define _SOUNDSYS_SOUND_SHADER_MANAGER_INTERFACE_HPP_

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

/// A global pointer to the sound shader manager.
/// This pointer should be used by all code that potentially is included in a DLL and should share the common sound shader manager
/// of another module (e.g. the main executable). As the DLL will during its initialization reset this pointer to the one provided by the
/// executable, this goal is thus automatically achieved.
extern SoundShaderManagerI* SoundShaderManager;

#endif
