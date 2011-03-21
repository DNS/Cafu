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

#ifndef _COMMAND_PASTE_HPP_
#define _COMMAND_PASTE_HPP_

#include "../CommandPattern.hpp"

#include "Math3D/Angles.hpp"
#include "Math3D/Vector3.hpp"


class MapDocumentT;
class MapElementT;
class MapWorldT;
class CommandSelectT;
class CommandAssignGroupT;
class CommandNewGroupT;


class CommandPasteT : public CommandT
{
    public:

    /// Constructor to paste an array of objects.
    /// @param MapDoc             The map document in which the pasting is done.
    /// @param Originals          The map elements that are pasted into the map document.
    /// @param OriginalsCenter    The center of the original objects.
    /// @param GoodPastePos       A good position for pasting the map elements. This is considered a suggestion, and adjusted when there are multiple subsequent pastes into the same place. When CenterAtOriginals is true, this parameter is ignored.
    /// @param DeltaTranslation   Translation offset for each pasted copy (only relevant if NumberOfCopies>1).
    /// @param DeltaRotation      Rotation offset for each pasted copy (only relevant if NumberOfCopies>1).
    /// @param NumberOfCopies     Number of times the objects are pasted into the world.
    /// @param PasteGrouped       Should all pasted objects be grouped?
    /// @param CenterAtOriginal   Should pasted objects be centered at position of original objects?
    CommandPasteT(MapDocumentT& MapDoc, const ArrayT<MapElementT*>& Originals, const Vector3fT& OriginalsCenter,
        const Vector3fT& GoodPastePos,
        const Vector3fT& DeltaTranslation=Vector3fT(), const cf::math::AnglesfT& DeltaRotation=cf::math::AnglesfT(),
        unsigned long NrOfCopies=1, bool PasteGrouped=false, bool CenterAtOriginals=false);

    ~CommandPasteT();

    // Implementation of the CommandT interface.
    bool     Do();
    void     Undo();
    wxString GetName() const;


    private:

    MapDocumentT&        m_MapDoc;              ///< The document we paste the map elements into.
    ArrayT<MapElementT*> m_PastedElems;         ///< The map elements that we pasted into the document.
    CommandSelectT*      m_CommandSelect;       ///< Subcommand for changing the selection.
    CommandNewGroupT*    m_CommandCreateGroup;  ///< Subcommand for creating a common group for the pasted elements.
    CommandAssignGroupT* m_CommandAssignGroup;  ///< Subcommand for putting the pasted elements in the newly created group.
};

#endif
