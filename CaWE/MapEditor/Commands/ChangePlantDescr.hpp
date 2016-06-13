/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_COMMAND_CHANGE_PLANT_DESCR_HPP_INCLUDED
#define CAFU_COMMAND_CHANGE_PLANT_DESCR_HPP_INCLUDED

#include "../../CommandPattern.hpp"


class MapDocumentT;
class MapPlantT;


class CommandChangePlantDescrT : public CommandT
{
    public:

    CommandChangePlantDescrT(MapDocumentT& MapDoc, MapPlantT* Plant, wxString NewPlantDescr);

    // CommandT implementation.
    bool Do();
    void Undo();
    wxString GetName() const;


    private:

    MapDocumentT&  m_MapDoc;
    MapPlantT*     m_Plant;
    const wxString m_NewPlantDescr;
    const wxString m_OldPlantDescr;
};

#endif
