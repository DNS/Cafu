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

#ifndef _EDITOR_MATERIAL_MANAGER_HPP_
#define _EDITOR_MATERIAL_MANAGER_HPP_

#include "MaterialSystem/MaterialManagerImpl.hpp"
#include "Templates/Array.hpp"
#include "wx/wx.h"


class EditorMaterialI;
class GameConfigT;


/// This class manages the editor materials for a game configuration.
class EditorMatManT
{
    public:

    EditorMatManT(const GameConfigT& GameConfig);
    ~EditorMatManT();

    const ArrayT<EditorMaterialI*>& GetMaterials() const { return m_Materials; }

    EditorMaterialI* GetDefaultMaterial();
    void             SetDefaultMaterial(EditorMaterialI* DefaultMat);

    /// This method finds an editor material by name.
    ///
    /// @param MatName       Name of the material to find.
    /// @param CreateDummy   Creates and returns a dummy material when the requested material was not found.
    ///
    /// @returns a pointer to the requested editor material.
    ///   If the material was not found and CreateDummy is true, a dummy material is added and its pointer is returned.
    ///   If the material was not found and CreateDummy is false, NULL is returned.
    EditorMaterialI* FindMaterial(const wxString& MatName, bool CreateDummy);

    // This loops through m_Materials, one per call, and calls the GetImage() method on the material,
    // in order to make sure that the proxy is fully loaded.
    void LazilyUpdateProxies();


    private:

    EditorMatManT(const EditorMatManT&);            ///< Use of the Copy Constructor    is not allowed.
    void operator = (const EditorMatManT&);         ///< Use of the Assignment Operator is not allowed.

    MaterialManagerImplT     m_MaterialMan;         ///< The material manager for the materials that are used with this game config.
    ArrayT<EditorMaterialI*> m_Materials;           ///< Array of all materials in this game configuration.
    EditorMaterialI*         m_DefaultMaterial;     ///< The currently used default material.
    unsigned long            m_LazyMatUpdateCount;  ///< This counts up to where we already cached in the m_Materials.
};

#endif
