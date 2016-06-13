/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_COMMAND_NEW_GROUP_HPP_INCLUDED
#define CAFU_COMMAND_NEW_GROUP_HPP_INCLUDED

#include "../../CommandPattern.hpp"


class GroupT;
class MapDocumentT;
class MapElementT;


/// This class implements a command for adding a new group to the map document.
/// The newly added group initially has no members: it's up to the caller to put map elements into the new group.
/// Having no members means that the new group initially doesn't affect (the visibility of) any map elements,
/// but also that it is possibly subject to auto-pruning if no members are added to it soon.
/// The counterpart to this class is CommandDeleteGroupT.
class CommandNewGroupT : public CommandT
{
    public:

    /// The constructor.
    /// @param MapDoc   The map document to add the new group to.
    /// @param Name     The name for the new group.
    CommandNewGroupT(MapDocumentT& MapDoc, const wxString& Name);

    /// The destructor.
    ~CommandNewGroupT();

    /// Returns the new group.
    GroupT* GetGroup() { return m_Group; }

    // Implementation of the CommandT interface.
    bool     Do();
    void     Undo();
    wxString GetName() const;


    private:

    MapDocumentT& m_MapDoc;
    GroupT*       m_Group;
};

#endif
