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

#ifndef _COMMAND_CHANGE_TERRAIN_RES_HPP_
#define _COMMAND_CHANGE_TERRAIN_RES_HPP_

#include "../CommandPattern.hpp"

#include "Templates/Array.hpp"


class MapDocumentT;
class MapTerrainT;


class CommandChangeTerrainResT : public CommandT
{
    public:

    /// Constructor to change a terrain resolution.
    /// @param MapDoc The map document in which the terrain is existent.
    /// @param Terrain Pointer to the terrain object that is to be modified.
    /// @param NewResolution The resolution this terrain is changed to.
    CommandChangeTerrainResT(MapDocumentT& MapDoc, MapTerrainT* Terrain, unsigned int NewResolution);

    // CommandT implementation.
    bool Do();
    void Undo();
    wxString GetName() const;


    private:

    MapDocumentT&                m_MapDoc;
    MapTerrainT*                 m_Terrain;
          ArrayT<unsigned short> m_NewHeightData;
    const ArrayT<unsigned short> m_PrevHeightData;
    const unsigned int           m_NewResolution;
    const unsigned int           m_PrevResolution;

    // Calculates the value of position x/y in the previous height data array as a bilinear interpolated value.
    unsigned short BilinearInterpolation(float x, float y);
};

#endif
