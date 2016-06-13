/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_SOUNDSYS_SOUND_SHADER_MANAGER_IMPLEMENTATION_HPP_INCLUDED
#define CAFU_SOUNDSYS_SOUND_SHADER_MANAGER_IMPLEMENTATION_HPP_INCLUDED

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
