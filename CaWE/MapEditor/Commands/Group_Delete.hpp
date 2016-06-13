/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_COMMAND_DELETE_GROUP_HPP_INCLUDED
#define CAFU_COMMAND_DELETE_GROUP_HPP_INCLUDED

#include "../../CommandPattern.hpp"


class CommandAssignGroupT;
class GroupT;
class MapDocumentT;


/// This class implements a command for deleting one or more groups from the map document.
/// Any members of the deleted group(s) are assigned the NULL (no) group.
/// The counterpart to this class is CommandNewGroupT.
class CommandDeleteGroupT : public CommandT
{
    public:

    /// The constructor for deleting a single group.
    /// @param MapDoc   The map document to delete the group from.
    /// @param Group    The group to be deleted from the map document.
    CommandDeleteGroupT(MapDocumentT& MapDoc, GroupT* Group);

    /// The constructor for deleting multiple groups at once.
    /// @param MapDoc   The map document to delete the groups from.
    /// @param Groups   The groups to be deleted from the map document.
    CommandDeleteGroupT(MapDocumentT& MapDoc, const ArrayT<GroupT*>& Groups);

    /// The destructor.
    ~CommandDeleteGroupT();

    // Implementation of the CommandT interface.
    bool Do();
    void Undo();
    wxString GetName() const;


    private:

    MapDocumentT&         m_MapDoc;           ///< The map document to delete the groups from.
    const ArrayT<GroupT*> m_DelGroups;        ///< The list of groups to delete.
    const ArrayT<GroupT*> m_OldGroups;        ///< The original list of groups before the delete.
    CommandAssignGroupT*  m_AssignNullGroup;
};

#endif
