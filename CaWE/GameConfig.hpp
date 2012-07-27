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

#ifndef CAFU_GAME_CONFIG_HPP_INCLUDED
#define CAFU_GAME_CONFIG_HPP_INCLUDED

#include "EditorMaterialManager.hpp"
#include "GuiSys/GuiResources.hpp"
#include "Math3D/BoundingBox.hpp"
#include "Models/ModelManager.hpp"
#include "Templates/Array.hpp"
#include "wx/wx.h"


namespace cf
{
    namespace FileSys
    {
        class FileSystemT;
    }
}

class EntityClassT;
class wxFileConfig;


/// The class describes the settings for a game/MOD.
/// Some of the settings are loaded from the CaWE-spefific config file (edited by the user in the main "Configure CaWE" dialog).
/// Some of the settings are loaded from the games EntityClassDefs.lua script.
/// Other settings are loaded directly from the files and data in the game/MOD directory.
class GameConfigT
{
    public:

    class InitErrorT { };

    GameConfigT(wxFileConfig& CfgFile, const wxString& Name_, const wxString& ModDir_);
    ~GameConfigT();

    const EntityClassT* FindClass(const wxString& Name) const;
    const ArrayT<const EntityClassT*>& GetEntityClasses() const { return m_EntityClasses; }
    EditorMatManT& GetMatMan() { return m_MatMan; }
    const EditorMatManT& GetMatMan() const { return m_MatMan; }

    /// Returns the model for the given FileName that is relative to ModDir.
    const CafuModelT* GetModel(const wxString& FileName, wxString* ErrorMsg=NULL) const;

    /// All GUIs that are created in this game config (no matter if in the Map Editor, the Gui Editor, or the Model Editor)
    /// share their font and model resources via the returned GuiResourcesT instance.
    cf::GuiSys::GuiResourcesT& GetGuiResources() { return m_GuiResources; }

    int GetMaxMapCoord() const { return  m_MaxMapCoord; }
    int GetMinMapCoord() const { return -m_MaxMapCoord; }
    BoundingBox3fT GetMaxMapBB() const;

    /// Saves this game configuration to CfgFile that has been set to the proper path (directory / group) by the caller.
    void Save(wxFileConfig& CfgFile) const;


    // Settings obtained from the CfgFile.
    const wxString Name;
    const wxString ModDir;

    wxString       DefaultPointEntity;
    wxString       DefaultSolidEntity;

    float          DefaultTextureScale;
    float          DefaultLightmapScale;
    wxString       CordonTexture;


    private:

    GameConfigT(const GameConfigT&);        ///< Use of the Copy Constructor    is not allowed.
    void operator = (const GameConfigT&);   ///< Use of the Assignment Operator is not allowed.

    ArrayT<cf::FileSys::FileSystemT*> m_MountedFileSystems;     ///< The file systems that have been mounted for this game config.
    ArrayT<const EntityClassT*>       m_EntityClasses;          ///< The entity classes as obtained from the EntityClassDefs.lua script.
    EditorMatManT                     m_MatMan;                 ///< The material manager for this game config.
    ModelManagerT                     m_ModelMan;               ///< The model manager for this game config.
    cf::GuiSys::GuiResourcesT         m_GuiResources;           ///< The provider for resources (fonts and models) for all GUIs in this game config (no matter if in the Map Editor, the Gui Editor, or the Model Editor).
    int                               m_MaxMapCoord;
};

#endif
