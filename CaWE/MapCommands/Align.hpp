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

#ifndef _COMMAND_ALIGN_HPP_
#define _COMMAND_ALIGN_HPP_

#include "../CommandPattern.hpp"
#include "../AxesInfo.hpp"
#include "Math3D/BoundingBox.hpp"


class MapElementT;
class MapDocumentT;


/// Command to align objects inside a specified box using an align mode.
class CommandAlignT : public CommandT
{
    public:

    /// Constructor to align an array of map elements.
    CommandAlignT(MapDocumentT& MapDoc, const ArrayT<MapElementT*>& Elems, const AxesInfoT& RefAxes, const BoundingBox3fT& Box, int Mode);

    /// Destructor.
    ~CommandAlignT();

    // Implementation of the CommandT interface.
    bool     Do();
    void     Undo();
    wxString GetName() const;


    private:

    MapDocumentT&              m_MapDoc;
    const ArrayT<MapElementT*> m_AlignElems;
    ArrayT<MapElementT*>       m_OldStates;
    const AxesInfoT            m_RefAxes;       ///< Axes of the 2D view in which the alignment is performed.
    const BoundingBox3fT       m_Box;           ///< Box in which the objects are aligned.
    const int                  m_Mode;

    /// Corrects the mode according to reference 2D view.
    int CorrectMode(int Mode) const;
};

#endif
