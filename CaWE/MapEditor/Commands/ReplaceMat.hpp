/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_COMMAND_REPLACE_MAT_HPP_INCLUDED
#define CAFU_COMMAND_REPLACE_MAT_HPP_INCLUDED

#include "../../CommandPattern.hpp"


class EditorMaterialI;
class MapBezierPatchT;
class MapBrushT;
class MapDocumentT;
class MapElementT;
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

    ArrayT<MapBrushT*>                 m_Brushes;
    ArrayT<MapBezierPatchT*>           m_BezierPatches;

    ArrayT< ArrayT<EditorMaterialI*> > m_OldBrushMats;
    ArrayT<EditorMaterialI*>           m_OldBezierPatchMats;

    MapDocumentT&        MapDoc;
    const wxString       Find;
    const wxString       Replace;
    const ReplaceActionT Action;
    const bool           MarkOnly;
    CommandSelectT*      CommandSelect;     ///< Subcommand used by this command to change the selection.
    wxString             m_ResultString;
};

#endif
