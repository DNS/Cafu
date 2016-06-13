/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_COMMAND_DELETE_HPP_INCLUDED
#define CAFU_COMMAND_DELETE_HPP_INCLUDED

#include "../../CommandPattern.hpp"
#include "Templates/Pointer.hpp"


namespace cf { namespace GameSys { class EntityT; } }
namespace MapEditor { class CompMapEntityT; }
class MapDocumentT;
class MapElementT;
class MapPrimitiveT;
class CommandSelectT;


class CommandDeleteT : public CommandT
{
    public:

    /// Constructor to delete an individual object.
    CommandDeleteT(MapDocumentT& MapDoc, MapElementT* DeleteElem);

    /// Constructor to delete an array of objects.
    CommandDeleteT(MapDocumentT& MapDoc, const ArrayT<MapElementT*>& DeleteElems);

    /// Destructor.
    ~CommandDeleteT();

    // Implementation of the CommandT interface.
    bool Do();
    void Undo();
    wxString GetName() const;


    private:

    void Init(const ArrayT<MapElementT*>& DeleteElems);

    MapDocumentT&                                      m_MapDoc;
    ArrayT< IntrusivePtrT<cf::GameSys::EntityT> >      m_Entities;            ///< The entities to delete.
    ArrayT< IntrusivePtrT<cf::GameSys::EntityT> >      m_EntityParents;       ///< The parents of the above entities.
    ArrayT<unsigned long>                              m_EntityIndices;       ///< The indices of the entities in their respective parent.
    ArrayT<MapPrimitiveT*>                             m_DeletePrims;         ///< The primitives to delete.
    ArrayT< IntrusivePtrT<MapEditor::CompMapEntityT> > m_DeletePrimsParents;  ///< The parents of the above primitives (the world or any custom entity).
    CommandSelectT*                                    m_CommandSelect;       ///< The command that unselects all elements before they are deleted.
};

#endif
