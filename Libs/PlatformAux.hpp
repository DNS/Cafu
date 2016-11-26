/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_PLATFORM_AUX_HPP_INCLUDED
#define CAFU_PLATFORM_AUX_HPP_INCLUDED

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#elif __linux__
#define HMODULE void*
#endif

#include <string>
#include <vector>

namespace MatSys
{
    class RendererI;
    class TextureMapManagerI;
}

class SoundSysI;


namespace PlatformAux
{
    /// Returns an array with the file and directory entries of the given directory.
    /// If 'f' is given as the filter argument, only entries of type file are returned,
    /// if 'd' is given as the filter argument, only entries of type directory are returned,
    /// all entries are returned in all other cases.
    std::vector<std::string> GetDirectory(const std::string& Name, char Filter=0);

    /// Gets the renderer by name.
    /// @param DLLName         The path and full name of the desired renderer DLL, e.g. "Renderers/RendererNull_win32_vc60_r.dll".
    /// @param OutRendererDLL  On success, returns the DLL handle of the renderer.
    /// @return   On success, the pointer to the implementation of the MatSys::RendererI. NULL on failure.
    MatSys::RendererI* GetRenderer(const std::string& DLLName, HMODULE& OutRendererDLL);

    /// Auto-selects the "best" renderer from the "Renderers/" subdirectory.
    /// @param OutRendererDLL On success, returns the DLL handle of the best renderer.
    /// @return   On success, the pointer to the implementation of the best MatSys::RendererI. NULL on failure.
    MatSys::RendererI* GetBestRenderer(HMODULE& OutRendererDLL);

    /// Returns the MatSys::TextureMapManagerI implementation of the RendererDLL.
    /// @param RendererDLL   The renderer DLL for which the texture manager should be returned.
    /// @return   On success the MatSys::TextureMapManagerI implementation of the RendererDLL. NULL on failure.
    MatSys::TextureMapManagerI* GetTextureMapManager(HMODULE RendererDLL);

    /// Gets the sound system by name.
    /// @param DLLName         The path and full name of the desired sound system DLL.
    /// @param OutSoundSysDLL  On success, returns the DLL handle of the sound system.
    /// @return On success, the pointer to the implementation of the SoundSysI. NULL on failure.
    SoundSysI* GetSoundSys(const std::string& DLLName, HMODULE& OutSoundSysDLL);

    /// Auto-selects the "best" sound system from the "SoundSys/" subdirectory.
    /// @param  OutSoundSysDLL On success, returns the DLL handle of the best sound system.
    /// @return On success, the pointer to the implementation of the best SoundSysI. NULL on failure.
    SoundSysI* GetBestSoundSys(HMODULE& OutSoundSysDLL);

}

#endif
