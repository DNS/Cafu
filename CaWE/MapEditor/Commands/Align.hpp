/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_COMMAND_ALIGN_HPP_INCLUDED
#define CAFU_COMMAND_ALIGN_HPP_INCLUDED

#include "../../CommandPattern.hpp"
#include "../../AxesInfo.hpp"
#include "Math3D/BoundingBox.hpp"


class MapDocumentT;
class MapElementT;
class TrafoMementoT;


/// Command to align objects inside a specified box using an align mode.
class CommandAlignT : public CommandT
{
    public:

    /// Constructor to align an array of map elements.
    CommandAlignT(MapDocumentT& MapDoc, const ArrayT<MapElementT*>& Elems, const AxesInfoT& RefAxes, const BoundingBox3fT& Box, int Mode, bool LockTexCoords);

    /// Destructor.
    ~CommandAlignT();

    // Implementation of the CommandT interface.
    bool     Do();
    void     Undo();
    wxString GetName() const;


    private:

    MapDocumentT&              m_MapDoc;
    const ArrayT<MapElementT*> m_AlignElems;
    ArrayT<TrafoMementoT*>     m_OldStates;
    const AxesInfoT            m_RefAxes;       ///< Axes of the 2D view in which the alignment is performed.
    const BoundingBox3fT       m_Box;           ///< Box in which the objects are aligned.
    const int                  m_Mode;
    const bool                 m_LockTexCoords;

    /// Corrects the mode according to reference 2D view.
    int CorrectMode(int Mode) const;
};

#endif
