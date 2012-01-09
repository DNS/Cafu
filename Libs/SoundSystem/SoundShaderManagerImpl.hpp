/*
=================================================================================
This file is part of Cafu, the open-source game engine and graphics engine
for multiplayer, cross-platform, real-time 3D action.
Copyright (C) 2002-2012 Carsten Fuchs Software.

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

#ifndef _SOUNDSYS_SOUND_SHADER_MANAGER_IMPLEMENTATION_HPP_
#define _SOUNDSYS_SOUND_SHADER_MANAGER_IMPLEMENTATION_HPP_

#include "SoundShaderManager.hpp"

#include <map>


class SoundShaderT;


/// Standard implementation of the sound shader manager.
class SoundShaderManagerImplT : public SoundShaderManagerI
{
    public:

    /// Constructor.
    SoundShaderManagerImplT();

    /// Destructor.
    ~SoundShaderManagerImplT();


    /// SoundShaderManagerI Implementation.
    ArrayT<const SoundShaderT*> RegisterSoundShaderScript(const std::string& ScriptFile, const std::string& ModDir);
    ArrayT<const SoundShaderT*> RegisterSoundShaderScriptsInDir(const std::string& Directory, const std::string& ModDir, bool Recurse=true);

    const SoundShaderT* GetSoundShader(const std::string& Name);


    private:

    SoundShaderManagerImplT(const SoundShaderManagerImplT&); // Use of the Copy Constructor    is not allowed.
    void operator = (const SoundShaderManagerImplT&);        // Use of the Assignment Operator is not allowed.

    ArrayT<std::string>                  m_SoundShaderScriptFiles; ///< Contains the names of all sound shader script files that have already been registered.
    std::map<std::string, SoundShaderT*> m_SoundShaders;           ///< Map of all registered sound shaders.
};

#endif
