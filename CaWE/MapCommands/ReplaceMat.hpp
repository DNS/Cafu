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

#ifndef _COMMAND_REPLACE_MAT_HPP_
#define _COMMAND_REPLACE_MAT_HPP_

#include "../CommandPattern.hpp"


class MapDocumentT;
class MapElementT;
class MapWorldT;
class CommandSelectT;


class CommandReplaceMatT : public CommandT
{
    public:

    enum ReplaceActionT
    {
        ExactMatches=0,         ///< Find and replace exact matches only.
        PartialMatchesFull,     ///< Find partial matches and replace the entire string.
        PartialMatchesSubst     ///< Find partial matches and replace the matching portion.
    };

    /// Constructor to replace or just mark materials in the current selection of the map document or all map objects.
    /// @param MapDoc_         The map document in which the replace/mark should be performed.
    /// @param Selection_      The set of elements to apply the mark or replace to.
    /// @param Find_           What should be found.
    /// @param Replace_        What it should be replaced with.
    /// @param Action_         The replace action to be taken.
    /// @param MarkOnly_       Whether matches should be replaced or just marked.
    /// @param SearchSelection Whether we should search the current selection or all objects.
    ///                        If this is true, the SearchBrushes, SearchBPatches and SearchHidden parameters have no effect.
    /// @param SearchBrushes   Whether brushes should be included in the search. Only effective if SearchSelection is false.
    /// @param SearchBPatches  Whether bezier patches should be included in the search. Only effective if SearchSelection is false.
    /// @param SearchHidden    Whether also hidden groups should be searched. Only effective if SearchSelection is false.
    CommandReplaceMatT(MapDocumentT& MapDoc_, const ArrayT<MapElementT*>& Selection_, const wxString& Find_, const wxString& Replace_,
        ReplaceActionT Action_, bool MarkOnly_, bool SearchSelection, bool SearchBrushes, bool SearchBPatches, bool SearchHidden);

    ~CommandReplaceMatT();

    /// Returns a string describing the result after the command has been run (that is, Do() has been called).
    /// Useful for user feedback after the first run.
    const wxString& GetResultString() const;

    // Implementation of the CommandT interface.
    bool     Do();
    void     Undo();
    wxString GetName() const;


    private:

    ArrayT<MapElementT*> ReplaceMatObjects;
    ArrayT<MapElementT*> OldStates;

    MapDocumentT&        MapDoc;
    const wxString       Find;
    const wxString       Replace;
    const ReplaceActionT Action;
    const bool           MarkOnly;
    CommandSelectT*      CommandSelect;     ///< Subcommand used by this command to change the selection.
    wxString             m_ResultString;
};

#endif
