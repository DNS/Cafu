/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_COMMAND_MODIFY_TERRAIN_HPP_INCLUDED
#define CAFU_COMMAND_MODIFY_TERRAIN_HPP_INCLUDED

#include "../../CommandPattern.hpp"


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
