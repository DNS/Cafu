/*
=================================================================================
This file is part of Cafu, the open-source game engine and graphics engine
for multiplayer, cross-platform, real-time 3D action.
Copyright (C) 2002-2012 Carsten Fuchs Software.

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

#ifndef _COMMAND_SET_BP_SUBDIVS_HPP_
#define _COMMAND_SET_BP_SUBDIVS_HPP_

#include "../CommandPattern.hpp"


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
