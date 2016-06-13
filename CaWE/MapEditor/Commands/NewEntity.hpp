/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_COMMAND_NEW_ENTITY_HPP_INCLUDED
#define CAFU_COMMAND_NEW_ENTITY_HPP_INCLUDED

#include "../../CommandPattern.hpp"
#include "Templates/Pointer.hpp"


namespace cf { namespace GameSys { class EntityT; } }
class CommandSelectT;
class MapDocumentT;


/// This commands inserts a new entity into the map.
class CommandNewEntityT : public CommandT
{
    public:

    /// The constructor.
    /// @param MapDoc   Map document into which the entity is inserted.
    /// @param Entity   The entity to insert.
    /// @param Parent   The parent entity.
    /// @param SetSel   Whether the inserted entity should automatically be selected.
    CommandNewEntityT(MapDocumentT& MapDoc, IntrusivePtrT<cf::GameSys::EntityT> Entity, IntrusivePtrT<cf::GameSys::EntityT> Parent, bool SetSel);

    /// The constructor.
    /// @param MapDoc     Map document into which the entities are inserted.
    /// @param Entities   The entities to insert.
    /// @param Parent     The parent entity.
    /// @param SetSel     Whether the inserted entities should automatically be selected.
    CommandNewEntityT(MapDocumentT& MapDoc, const ArrayT< IntrusivePtrT<cf::GameSys::EntityT> >& Entities, IntrusivePtrT<cf::GameSys::EntityT> Parent, bool SetSel=true);

    /// The destructor.
    ~CommandNewEntityT();

    // Implementation of the CommandT interface.
    bool     Do();
    void     Undo();
    wxString GetName() const;


    private:

    MapDocumentT&                                 m_MapDoc;
    ArrayT< IntrusivePtrT<cf::GameSys::EntityT> > m_Entities;
    IntrusivePtrT<cf::GameSys::EntityT>           m_Parent;
    const bool                                    m_SetSel;
    CommandSelectT*                               m_CommandSelect;  ///< Subcommand for changing the selection.
};

#endif
