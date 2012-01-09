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

#ifndef _COMMAND_SELECT_HPP_
#define _COMMAND_SELECT_HPP_

#include "../CommandPattern.hpp"


class MapDocumentT;
class MapElementT;


class CommandSelectT : public CommandT
{
    public:

    // Named constructors for easier command creation.
    static CommandSelectT* Clear (MapDocumentT* MapDocument);
    static CommandSelectT* Add   (MapDocumentT* MapDocument, const ArrayT<MapElementT*>& MapElements);
    static CommandSelectT* Add   (MapDocumentT* MapDocument, MapElementT* MapElement);
    static CommandSelectT* Remove(MapDocumentT* MapDocument, const ArrayT<MapElementT*>& MapElements);
    static CommandSelectT* Remove(MapDocumentT* MapDocument, MapElementT* MapElement);
    static CommandSelectT* Set   (MapDocumentT* MapDocument, const ArrayT<MapElementT*>& MapElements);
    static CommandSelectT* Set   (MapDocumentT* MapDocument, MapElementT* MapElement);

    ~CommandSelectT();

    // CommandT implementation.
    bool Do();
    void Undo();
    wxString GetName() const;


    private:

    // Only named constructors may create this command.
    CommandSelectT(MapDocumentT* MapDocument, const ArrayT<MapElementT*>& OldSelection, const ArrayT<MapElementT*>& NewSelection);

    MapDocumentT* m_MapDocument;

    const ArrayT<MapElementT*> m_OldSelection;
    const ArrayT<MapElementT*> m_NewSelection;
};

#endif
