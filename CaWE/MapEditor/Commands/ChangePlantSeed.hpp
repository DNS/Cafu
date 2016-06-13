/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_COMMAND_CHANGE_PLANT_SEED_HPP_INCLUDED
#define CAFU_COMMAND_CHANGE_PLANT_SEED_HPP_INCLUDED

#include "../../CommandPattern.hpp"


class MapDocumentT;
class MapPlantT;


class CommandChangePlantSeedT : public CommandT
{
    public:

    CommandChangePlantSeedT(MapDocumentT& MapDoc, MapPlantT* Plant, unsigned int NewRandomSeed);

    // CommandT implementation.
    bool Do();
    void Undo();
    wxString GetName() const;


    private:

    MapDocumentT&      m_MapDoc;
    MapPlantT*         m_Plant;
    const unsigned int m_NewRandomSeed;
    const unsigned int m_OldRandomSeed;
};

#endif
