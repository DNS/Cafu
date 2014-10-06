/*
=================================================================================
This file is part of Cafu, the open-source game engine and graphics engine
for multiplayer, cross-platform, real-time 3D action.
Copyright (C) 2002-2014 Carsten Fuchs Software.

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

#ifndef CAFU_COMMAND_APPLY_MATERIAL_HPP_INCLUDED
#define CAFU_COMMAND_APPLY_MATERIAL_HPP_INCLUDED

#include "../../CommandPattern.hpp"


class MapBrushT;
class MapBezierPatchT;
class MapDocumentT;
class MapElementT;
class MapTerrainT;
class EditorMaterialI;


class CommandApplyMaterialT : public CommandT
{
    public:

    /// Constructor to apply the given material to an array of objects.
    CommandApplyMaterialT(MapDocumentT& MapDoc, const ArrayT<MapElementT*>& Objects, EditorMaterialI* Material);

    // Implementation of the CommandT interface.
    bool     Do();
    void     Undo();
    wxString GetName() const;


    private:

    MapDocumentT&                      m_MapDoc;
    EditorMaterialI*                   m_Material;
    ArrayT<MapElementT*>               m_Objects;

    ArrayT<MapBrushT*>                 m_Brushes;
    ArrayT<MapBezierPatchT*>           m_BezierPatches;
    ArrayT<MapTerrainT*>               m_Terrains;

    ArrayT< ArrayT<EditorMaterialI*> > m_OldBrushMats;
    ArrayT<EditorMaterialI*>           m_OldBezierPatchMats;
    ArrayT<EditorMaterialI*>           m_OldTerrainMats;
};

#endif
