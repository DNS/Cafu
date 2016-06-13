/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_COMMAND_CHANGE_TERRAIN_RES_HPP_INCLUDED
#define CAFU_COMMAND_CHANGE_TERRAIN_RES_HPP_INCLUDED

#include "../../CommandPattern.hpp"

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
