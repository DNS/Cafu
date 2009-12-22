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

#ifndef _COMMAND_MIRROR_HPP_
#define _COMMAND_MIRROR_HPP_

#include "../CommandPattern.hpp"


class MapDocumentT;
class MapElementT;


/// Command to mirror map elements along a given mirror plane.
class CommandMirrorT : public CommandT
{
    public:

    /// The constructor.
    CommandMirrorT(MapDocumentT& MapDoc, const ArrayT<MapElementT*>& Elems, unsigned int NormalAxis, float Dist);

    /// Destructor.
    ~CommandMirrorT();

    // Implementation of the CommandT interface.
    bool     Do();
    void     Undo();
    wxString GetName() const;


    private:

    MapDocumentT&              m_MapDoc;
    const ArrayT<MapElementT*> m_MirrorElems;
    ArrayT<MapElementT*>       m_OldStates;
    const unsigned int         m_NormalAxis;    ///< The number of the axis along which the normal vector of the mirror plane points: 0, 1 or 2 for the x-, y- or z-axis respectively.
    const float                m_Dist;          ///< The position of the mirror plane along its normal vector, where it intersects the NormalAxis.
};

#endif
