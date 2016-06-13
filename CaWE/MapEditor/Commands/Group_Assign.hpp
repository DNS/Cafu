/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_COMMAND_ASSIGN_GROUP_HPP_INCLUDED
#define CAFU_COMMAND_ASSIGN_GROUP_HPP_INCLUDED

#include "../../CommandPattern.hpp"


class CommandSelectT;
class GroupT;
class MapDocumentT;
class MapElementT;


/// This class implements a command for putting a set of given map elements into a given group ("MapElems[i].Group=NewGroup").
/// If the given group is hidden, any selected map elements are automatically unselected.
class CommandAssignGroupT : public CommandT
{
    public:

    /// The constructor.
    /// @param MapDoc     The relevant map document.
    /// @param MapElems   The map elements that are to be put into the given group.
    /// @param Group      The group that the MapElems are put into. Can be NULL for "no group".
    CommandAssignGroupT(MapDocumentT& MapDoc, const ArrayT<MapElementT*>& MapElems, GroupT* Group);

    // Implementation of the CommandT interface.
    bool Do();
    void Undo();
    wxString GetName() const;


    private:

    MapDocumentT&        m_MapDoc;
    ArrayT<MapElementT*> m_MapElems;
    GroupT*              m_Group;
    ArrayT<GroupT*>      m_PrevGroups;  ///< The m_MapElems previous groups: In which group was m_MapElems[i] before our Do() put it into m_Group? Used for Undo().
    ArrayT<MapElementT*> m_VisChanged;  ///< The elements from m_MapElems whose visibility changed due to their being put into m_Group.
    CommandSelectT*      m_CommandReduceSel;
};

#endif
