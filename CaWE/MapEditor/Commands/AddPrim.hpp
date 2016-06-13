/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_COMMAND_ADD_PRIMITIVE_HPP_INCLUDED
#define CAFU_COMMAND_ADD_PRIMITIVE_HPP_INCLUDED

#include "../../CommandPattern.hpp"
#include "Templates/Pointer.hpp"


namespace MapEditor { class CompMapEntityT; }
class MapDocumentT;
class MapElementT;
class MapPrimitiveT;
class CommandSelectT;


/// This class implements a command for adding primitives into the map, as part of their specified parent entities.
/// It is quasi the counterpart to CommandDeleteT.
class CommandAddPrimT : public CommandT
{
    public:

    /// The constructor for creating a command for adding a single primitive into the map.
    /// @param MapDoc    The map document into which the primitive is inserted.
    /// @param AddPrim   The primitive to add. NOTE: AddPrim->GetParent()==NULL is assumed, i.e. AddPrim has no prior parent!
    /// @param Parent    The parent entity that the primitive becomes a part of and which it is added to. Can be the world or a custom entity.
    /// @param Name      The name of this command for the undo history.
    /// @param SetSel    Whether the selection should be set to the newly added map primitive(s).
    CommandAddPrimT(MapDocumentT& MapDoc, MapPrimitiveT* AddPrim, IntrusivePtrT<MapEditor::CompMapEntityT> Parent, wxString Name="new primitive", bool SetSel=true);

    /// The constructor for creating a command for adding multiple primitives into the map.
    /// Analogous to the constructor for adding a single primitive above.
    CommandAddPrimT(MapDocumentT& MapDoc, const ArrayT<MapPrimitiveT*>& AddPrims, IntrusivePtrT<MapEditor::CompMapEntityT> Parent, wxString Name="new primitives", bool SetSel=true);

    /// The destructor.
    ~CommandAddPrimT();

    // Implementation of the CommandT interface.
    bool     Do();
    void     Undo();
    wxString GetName() const;


    private:

    MapDocumentT&                            m_MapDoc;
    ArrayT<MapPrimitiveT*>                   m_AddPrims;
    IntrusivePtrT<MapEditor::CompMapEntityT> m_Parent;
    CommandSelectT*                          m_CommandSelect; ///< Subcommand for changing the selection.
    wxString                                 m_Name;
};

#endif
