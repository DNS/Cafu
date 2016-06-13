/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_COMMAND_CARVE_HPP_INCLUDED
#define CAFU_COMMAND_CARVE_HPP_INCLUDED

#include "../../CommandPattern.hpp"
#include "Templates/Pointer.hpp"


namespace MapEditor { class CompMapEntityT; }
class CommandDeleteT;
class GroupT;
class MapDocumentT;
class MapBrushT;
class MapElementT;


class CommandCarveT : public CommandT
{
    public:

    /// Constructor to carve an array of objects from the world.
    CommandCarveT(MapDocumentT& MapDoc, const ArrayT<const MapBrushT*>& Carvers);

    ~CommandCarveT();

    // Implementation of the CommandT interface.
    bool Do();
    void Undo();
    wxString GetName() const;


    private:

    MapDocumentT&                                      m_MapDoc;
    const ArrayT<const MapBrushT*>                     m_Carvers;
    CommandDeleteT*                                    m_DeleteCommand;     ///< Subcommand to delete the original brushes that are carved.
    ArrayT<GroupT*>                                    m_NewCarveGroups;    ///< One new group for the carve pieces of each original brush, when the original brush was in no group before and carved into at least two pieces.

    ArrayT<MapElementT*>                               m_OriginalBrushes;   ///< The affected brushes before they were carved.
    ArrayT< IntrusivePtrT<MapEditor::CompMapEntityT> > m_Parents;           ///< Parent entities of the original brushes.
    ArrayT< ArrayT<MapBrushT*> >                       m_CarvedBrushes;     ///< For each original brush, the brushes resulting from the carve operation.
};

#endif
