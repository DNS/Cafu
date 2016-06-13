/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_COMMAND_SET_BP_SUBDIVS_HPP_INCLUDED
#define CAFU_COMMAND_SET_BP_SUBDIVS_HPP_INCLUDED

#include "../../CommandPattern.hpp"


class MapBezierPatchT;
class MapDocumentT;


enum SubdivDirE
{
    HORIZONTAL,
    VERTICAL
};


class CommandSetBPSubdivsT : public CommandT
{
    public:

    /// Constructor.
    CommandSetBPSubdivsT(MapDocumentT* MapDoc, MapBezierPatchT* BezierPatch, int Amount, SubdivDirE Direction);

    // Implementation of the CommandT interface.
    bool Do();
    void Undo();
    wxString GetName() const;


    private:

    MapDocumentT*    m_MapDoc;
    MapBezierPatchT* m_BezierPatch;

    const int        m_NewAmount;
    const int        m_OldAmount;
    const SubdivDirE m_Direction;
};

#endif
