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

#ifndef _COMMAND_MODIFY_TERRAIN_HPP_
#define _COMMAND_MODIFY_TERRAIN_HPP_

#include "../CommandPattern.hpp"


class MapDocumentT;
class MapTerrainT;


class CommandModifyTerrainT : public CommandT
{
    public:

    /// Constructor to modify a terrain with given new height data.
    /// The terrains current height data is overwritten with the passed new height data.
    /// Note that the dimensions of the passed NewHeightData cannot be greater than the dimensions of the current height data.
    /// @param MapDoc The map document in which the terrain is existent.
    /// @param Terrain Pointer to the terrain object that is to be modified.
    /// @param NewHeightData The height data with which the terrain is modified.
    /// @param Position The position in the terrains current height data. Beginning at this position the height data is overwritten with the new data from NewHeightData.
    /// @param SizeX X size of the new height data.
    /// @param SizeY Y size of the new height data.
    CommandModifyTerrainT(MapDocumentT& MapDoc, MapTerrainT* Terrain, const ArrayT<unsigned short>& NewHeightData, const wxPoint& Position, unsigned int SizeX, unsigned int SizeY);

    // CommandT implementation.
    bool Do();
    void Undo();
    wxString GetName() const;


    private:

    MapDocumentT&                m_MapDoc;         ///< The map document in which the terrain is existent.
    MapTerrainT*                 m_Terrain;        ///< The terrain object that is to be modified.
    const ArrayT<unsigned short> m_NewHeightData;  ///< The height data with which the terrain is modified.
          ArrayT<unsigned short> m_PrevHeightData; ///< Backup of the terrains previous heigth data.
    const wxPoint                m_Offset;         ///< The x/y offset in the terrains current height data.
    const unsigned int           m_SizeX;          ///< SizeX X size of the new height data.
    const unsigned int           m_SizeY;          ///< SizeY Y size of the new height data.
};

#endif
