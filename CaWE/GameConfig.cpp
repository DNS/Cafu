/*
=================================================================================
This file is part of Cafu, the open-source game engine and graphics engine
for multiplayer, cross-platform, real-time 3D action.
Copyright (C) 2002-2013 Carsten Fuchs Software.

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

#include "GameConfig.hpp"
#include "FileSys/FileMan.hpp"
#include "wx/fileconf.h"


GameConfigT::GameConfigT(wxFileConfig& CfgFile, const wxString& Name_, const wxString& ModDir_)
    : Name(Name_),
      ModDir(ModDir_),
      m_MatMan(*this),
      m_ModelMan(),
      m_GuiResources(m_ModelMan),
      m_MaxMapCoord(8192)   // This is reset below!
{
    // The next line doesn't work with wx 2.6.4, it always returns the default value 1.0.
 // DefaultTextureScale =CfgFile.Read("DefaultTextureScale", 1.0);
    double d;
    CfgFile.Read("DefaultTextureScale",  &d, 0.25); DefaultTextureScale =d;     // Work-around...
    CfgFile.Read("DefaultLightmapScale", &d, 16.0); DefaultLightmapScale=d;

    CordonTexture = CfgFile.Read("CordonTexture", "Textures/generic/_orange");


    // Mount the file systems relevant for this MOD.
    const std::string ModDirSlash(ModDir+"/");

    m_MountedFileSystems.PushBack(cf::FileSys::FileMan->MountFileSystem(cf::FileSys::FS_TYPE_LOCAL_PATH, ModDirSlash, ModDirSlash));
    m_MountedFileSystems.PushBack(cf::FileSys::FileMan->MountFileSystem(cf::FileSys::FS_TYPE_ZIP_ARCHIVE, ModDirSlash+"Textures/TechDemo.zip", ModDirSlash+"Textures/TechDemo/", "Ca3DE"));
    m_MountedFileSystems.PushBack(cf::FileSys::FileMan->MountFileSystem(cf::FileSys::FS_TYPE_ZIP_ARCHIVE, ModDirSlash+"Textures/SkyDomes.zip", ModDirSlash+"Textures/SkyDomes/", "Ca3DE"));


    // The old EntityClassDefs.lua file had this comment about the map size:
    // -- Let DeathMatch maps be up to 131072 (65536*2) units big in each direction. This corresponds to a side length of 3329,23 km.
    // -- Note: If this gets any bigger than 65536, CaWE needs revision, especially cmap brush loading (starting digging from LoadSave_cmap.cpp)!
    // Mapsize={ -65536, 65536 }
    m_MaxMapCoord = 65536;

    // Maybe we should revive the EntityClassDefs.lua file later, e.g. as GameConfig.lua,
    // and put definitions like mounted file systems and MaxMapCoord etc. into it...?
}


GameConfigT::~GameConfigT()
{
    if (cf::FileSys::FileMan!=NULL)
    {
        for (unsigned long MFSNr=0; MFSNr<m_MountedFileSystems.Size(); MFSNr++)
            cf::FileSys::FileMan->Unmount(m_MountedFileSystems[MFSNr]);
    }

    m_MountedFileSystems.Clear();
}


const CafuModelT* GameConfigT::GetModel(const wxString& FileName, wxString* ErrorMsg) const
{
    std::string       Msg;
    const CafuModelT* Model=m_ModelMan.GetModel(std::string(ModDir + "/" + FileName), &Msg);

    if (ErrorMsg)
        *ErrorMsg=Msg;

    return Model;
}


BoundingBox3fT GameConfigT::GetMaxMapBB() const
{
    const float f=m_MaxMapCoord;

    return BoundingBox3fT(Vector3fT(-f, -f, -f), Vector3fT(f, f, f));
}


void GameConfigT::Save(wxFileConfig& CfgFile) const
{
    CfgFile.DeleteGroup(".");

    CfgFile.Write("DefaultTextureScale",  DefaultTextureScale);
    CfgFile.Write("DefaultLightmapScale", DefaultLightmapScale);

    CfgFile.Write("CordonTexture", CordonTexture);
}
