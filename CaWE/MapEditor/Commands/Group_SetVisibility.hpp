/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_COMMAND_GROUP_SET_VISIBILITY_HPP_INCLUDED
#define CAFU_COMMAND_GROUP_SET_VISIBILITY_HPP_INCLUDED

#include "../../CommandPattern.hpp"


class CommandSelectT;
class GroupT;
class MapDocumentT;


/// This class implements a command for setting the visibility status of a group.
/// The current selection is automatically reduced to visible elements only, that is,
/// selected map elements that are hidden become automatically unselected.
class CommandGroupSetVisibilityT : public CommandT
{
    public:

    /// The constructor.
    /// @param MapDoc   The map document the group is in.
    /// @param Group    The group whose visibility is set.
    /// @param NewVis   The new visibility status for the group. If Group->IsVisible is already NewVis, Do() will fail.
    CommandGroupSetVisibilityT(MapDocumentT& MapDoc, GroupT* Group, bool NewVis);

    /// The destructor.
    ~CommandGroupSetVisibilityT();

    /// Returns the group whose visibility is set.
    const GroupT* GetGroup() const { return m_Group; }

    // Implementation of the CommandT interface.
    bool Do();
    void Undo();
    wxString GetName() const;


    private:

    MapDocumentT&   m_MapDoc;
    GroupT*         m_Group;
    const bool      m_NewVis;
    CommandSelectT* m_CommandReduceSel;
};

#endif
