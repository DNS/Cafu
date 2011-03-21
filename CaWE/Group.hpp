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

#ifndef _GROUP_HPP_
#define _GROUP_HPP_

#include "Templates/Array.hpp"
#include "wx/wx.h"


class MapDocumentT;
class MapElementT;
class TextParserT;


/// This class represents groups.
class GroupT
{
    public:

    /// The constructor.
    GroupT(const wxString& Name_);

    /// Named constructor for creating a group from a cmap file.
    static GroupT Create_cmap(TextParserT& TP);

    void Save_cmap(std::ostream& OutFile, unsigned long GroupNr) const;

    ArrayT<MapElementT*> GetMembers(const MapDocumentT& MapDoc) const;  ///< Returns all map elements in this group.
    bool                 HasMembers(const MapDocumentT& MapDoc) const;  ///< Returns whether this group has members (GetMembers().Size()>0, but more efficient).

    wxString Name;
    wxColour Color;
    bool     IsVisible;     ///< Are the members of this group visible in the (2D and 3D) views?
    bool     CanSelect;     ///< Can the members of this group be selected in the (2D and 3D) views? When false, the members are considered "locked" in order to prevent their editing.
    bool     SelectAsGroup; ///< Are the members of this group selected "as a group" (normal group behaviour) or can they be selected individually?
};

#endif
